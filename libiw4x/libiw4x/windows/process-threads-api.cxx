#include <libiw4x/windows/init.hxx>

namespace iw4x
{
  namespace windows
  {
    // MinGW does not guarantee orderly shutdown for C++ static objects that
    // live in a DLL. The problem originates in how destructors are
    // registered. On Windows, DLLs do not participate in the atexit() chain.
    // Only the main executable does. This is intentional: DLLs can be
    // unloaded at arbitrary times, so allowing them to register atexit()
    // destructors would leave the runtime with stale function pointers.
    // Invoking them during final process teardown would dereference code that
    // no longer exists.
    //
    // The direct consequence here is that static-duration objects inside a
    // DLL never receive guaranteed destruction. Under MSVC this is usually
    // benign because thread finalization and CRT teardown follow a more
    // predictable path. Under MinGW the situation is markedly worse: the
    // runtime attempts a full unwind of thread-local and static resources
    // during ExitProcess(), but the DLL boundary confuses its finalizer and
    // stall in the shutdown path, causing the process to hang indefinitely.
    //
    void WINAPI
    exit_process (UINT code)
    {
      HANDLE p (GetCurrentProcess ());
      TerminateProcess (p, code);
    }
  }
}
