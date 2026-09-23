#pragma once

#include <cstdint>
#include <cstddef>
#include <concepts>

#include <windows.h>

#include <libiw4x/contract.hxx>

namespace iw4x
{
  namespace gdk
  {
    class mutex
    {
    public:
      mutex () noexcept = default;

      mutex (const mutex&) = delete;
      mutex& operator= (const mutex&) = delete;

      void
      acquire () noexcept
      {
        AcquireSRWLockExclusive (&lock_);
      }

      void
      release () noexcept
      {
        ReleaseSRWLockExclusive (&lock_);
      }

      SRWLOCK*
      native () noexcept
      {
        return &lock_;
      }

    private:
      SRWLOCK lock_ = SRWLOCK_INIT;
    };

    class scope_lock
    {
    public:
      explicit
      scope_lock (mutex& m) noexcept: mutex_ (&m), held_ (true)
      {
        mutex_->acquire ();
      }

      ~scope_lock ()
      {
        if (held_)
          mutex_->release ();
      }

      scope_lock (const scope_lock&) = delete;
      scope_lock& operator= (const scope_lock&) = delete;

      void
      release () noexcept
      {
        if (held_)
        {
          mutex_->release ();
          held_ = false;
        }
      }

      mutex&
      owner () const noexcept
      {
        return *mutex_;
      }

    private:
      mutex* mutex_;
      bool   held_;
    };

    class condition
    {
    public:
      condition () noexcept = default;

      condition (const condition&) = delete;
      condition& operator= (const condition&) = delete;

      bool
      wait (scope_lock& l, DWORD timeout) noexcept
      {
        return SleepConditionVariableSRW (&condition_,
                                          l.owner ().native (),
                                          timeout,
                                          0) != 0;
      }

      void
      wake_one () noexcept
      {
        WakeConditionVariable (&condition_);
      }

      void
      wake_all () noexcept
      {
        WakeAllConditionVariable (&condition_);
      }

    private:
      CONDITION_VARIABLE condition_ = CONDITION_VARIABLE_INIT;
    };

    class thread
    {
    public:
      thread () noexcept = default;

      thread (const thread&) = delete;
      thread& operator= (const thread&) = delete;

      ~thread ()
      {
        join ();
      }

      bool
      start (void (*body) (void*), void* context) noexcept
      {
        LIBIW4X_PRE (body != nullptr);
        LIBIW4X_PRE (!started ());

        handle_ = CreateThread (nullptr, 0, &run, new_entry (body, context), 0,
                                nullptr);

        return handle_ != nullptr;
      }

      bool
      started () const noexcept
      {
        return handle_ != nullptr;
      }

      void
      join () noexcept
      {
        if (handle_ == nullptr)
          return;

        WaitForSingleObject (handle_, INFINITE);
        CloseHandle (handle_);

        handle_ = nullptr;
      }

    private:
      struct entry
      {
        void (*body) (void*);
        void*  context;
      };

      entry*
      new_entry (void (*b) (void*), void* c) noexcept
      {
        entry_ = entry {b, c};

        return &entry_;
      }

      static DWORD WINAPI
      run (LPVOID p) noexcept
      {
        const entry& e (*static_cast<entry*> (p));

        e.body (e.context);

        return 0;
      }

      entry  entry_ {nullptr, nullptr};
      HANDLE handle_ = nullptr;
    };

    inline std::uint64_t
    now () noexcept
    {
      return GetTickCount64 ();
    }
  }
}
