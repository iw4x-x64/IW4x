#include <libiw4x/win32/process-threads-api.hxx>

#include <libiw4x/iw4x.hxx>

#include <atomic>

using namespace std;

extern "C" BOOL WINAPI _CRT_INIT (HANDLE, DWORD, LPVOID);

namespace iw4x
{
  namespace win32
  {
    namespace
    {
      struct termination
      {
        static constexpr unsigned long deadline = 5000;

        void (WINAPI* original) (UINT) {nullptr};

        atomic <unsigned>      code     {0};
        atomic <unsigned long> owner    {0};
        atomic <bool>          complete {false};
      };

      // The one instance, reached from the replacement below.
      //
      // ExitProcess does not give us anywhere to pass our state. There
      // is only one termination object for the lifetime of the runtime,
      // which is also what makes this fairly simple.
      //
      termination exit_process_state;
    }

    LPVOID*
    exit_process_original ()
    {
      return reinterpret_cast <LPVOID*> (&exit_process_state.original);
    }

    // Run global destruction.
    //
    // This is essentially the part of the normal CRT detach that we
    // need to move in front of ExitProcess. In MinGW _CRT_INIT on
    // DLL_PROCESS_DETACH processes the on-exit table which, among other
    // things, contains registrations made by atexit and __cxa_atexit as
    // well as __do_global_dtors.
    //
    // An interesting detail here is that we don't try to reproduce any
    // of this ourselves. Apart from being rather fragile, the CRT
    // already has the state needed to make this idempotent
    // (__native_startup_state, __proc_attached, etc). So call back into
    // it and let it decide what is still left to do.
    //
    // Note also that we are still before the real ExitProcess at this
    // point. This is important since destructors are allowed to expect
    // the process, and in particular its other threads, to still be
    // alive.
    //
    static void
    destruct ()
    {
      _CRT_INIT (module (), DLL_PROCESS_DETACH, nullptr);
    }

    static DWORD WINAPI
    cleanup (LPVOID)
    {
      // There isn't much to do on this thread besides running the CRT
      // detach. The reason for having the thread at all is on the other
      // side: whoever asked the process to exit remains free to put a
      // deadline around this call.
      //
      destruct ();

      // Publish this only after _CRT_INIT has returned. The other
      // threads use it as the indication that it is now safe to
      // continue with native process termination.
      //
      exit_process_state.complete.store (true, memory_order_release);

      return 0;
    }

    void WINAPI
    exit_process (UINT c)
    {
      termination& t (exit_process_state);

      unsigned long self (GetCurrentThreadId ());

      // Eventually we have to continue through the ExitProcess that
      // MinHook saved for us. Keep that operation in one place since
      // there are a few paths below that end up there.
      //
      // In theory ExitProcess never returns. Still, if the trampoline
      // does return for whatever reason, then returning from our
      // replacement is the worst thing we could do: the caller has
      // already decided that the process is finished. So make
      // TerminateProcess the last-resort backstop.
      //
      auto transfer ([] (UINT c) -> void
      {
        t.original (c);

        TerminateProcess (GetCurrentProcess (), c);
      });

      // First deal with re-entry by the thread that already owns
      // termination. We cannot wait for ourselves below, and there is
      // no new cleanup work for such a request to contribute anyway.
      //
      // Note that the exit code belongs to the first request. Once
      // termination is in progress we keep using that code instead of
      // allowing a later call to change the result depending on which
      // thread happened to get here last.
      //
      if (t.owner.load (memory_order_acquire) == self)
        transfer (static_cast <UINT> (t.code.load (memory_order_relaxed)));

      // Now try to become the thread responsible for termination. Zero
      // is the unclaimed value and compare_exchange gives exactly one
      // caller ownership. Everyone else falls through to the waiting
      // case below.
      //
      unsigned long none (0);

      if (t.owner.compare_exchange_strong (none,
                                           self,
                                           memory_order_acq_rel,
                                           memory_order_acquire))
      {
        // We won, so remember the exit code before doing anything that
        // another terminating thread may have to observe.
        //
        t.code.store (c, memory_order_relaxed);

        // The slightly tricky part is running destructors without
        // giving them an unlimited amount of time to finish. Calling
        // destruct() here would leave us stuck as soon as a destructor
        // blocks, with nobody left to enforce the deadline.
        //
        // So run the detach on a worker and keep this thread as the
        // waiter. This also means the process has not entered native
        // termination while the destructors are running, and all the
        // threads they may depend on are still around.
        //
        HANDLE h (CreateThread (nullptr, 0, &cleanup, nullptr, 0, nullptr));

        if (h == nullptr)
        {
          // Without the cleanup thread we can no longer have both
          // destruction and a bounded wait. Running destruction inline
          // would lose the bound, so there isn't a useful
          // controlled-shutdown path left here.
          //
          TerminateProcess (GetCurrentProcess (), c);
        }

        // Wait for the complete detach sequence. cleanup() only returns
        // after destruct() has returned, so WAIT_OBJECT_0 means there
        // is no destructor still running on that thread.
        //
        DWORD w (WaitForSingleObject (h, termination::deadline));

        CloseHandle (h);

        if (w != WAIT_OBJECT_0)
        {
          // Presumably this is a destructor that did not finish in
          // time, though a failed wait leads to the same conclusion for
          // us: we cannot safely continue with the normal termination
          // sequence.
          //
          // In particular, don't call ExitProcess while the cleanup
          // thread may still be inside CRT detach. That could start
          // loader detach processing while user destruction is still
          // touching the same process state.
          //
          TerminateProcess (GetCurrentProcess (), c);
        }
      }
      else
      {
        // Another thread got here first and is doing the cleanup. We
        // still cannot call the original ExitProcess yet since that
        // would pull the process out from under its destructors, so
        // wait until it publishes completion.
        //
        // We don't have the cleanup thread handle on this path. Polling
        // the shared completion flag is enough and, more importantly,
        // lets us apply the same finite deadline instead of trusting
        // the owner to eventually make progress.
        //
        for (unsigned long i (0); i < termination::deadline &&
             !t.complete.load (memory_order_acquire);
             i += 10)
          Sleep (10);

        if (!t.complete.load (memory_order_acquire))
        {
          // The owner did not manage to finish cleanup in time. Use its
          // recorded code so that the first exit request remains
          // canonical.
          //
          TerminateProcess (
            GetCurrentProcess (),
            static_cast <UINT> (t.code.load (memory_order_relaxed)));
        }
      }

      // Cleanup has finished, either by us or by the thread that won
      // the race. Continue through the saved ExitProcess trampoline and
      // let Windows perform the ordinary process teardown from here.
      //
      transfer (static_cast <UINT> (t.code.load (memory_order_relaxed)));
    }
  }
}
