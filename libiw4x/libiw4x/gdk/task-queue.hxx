#pragma once

#include <atomic>
#include <cstdint>
#include <cstddef>

#include <libiw4x/contract.hxx>

#include <libiw4x/gdk/sync.hxx>
#include <libiw4x/gdk/types.hxx>

namespace iw4x
{
  namespace gdk
  {
    enum class port: std::uint32_t
    {
      work       = 0,
      completion = 1
    };

    enum class dispatch_mode: std::uint32_t
    {
      manual                 = 0,
      async                  = 1,
      thread_pool            = 2,
      serialized_thread_pool = 3,
      immediate              = 4
    };

    using task = void (*) (void* context, bool canceled);

    class task_port
    {
    public:
      static constexpr std::uint32_t capacity    = 128;
      static constexpr std::uint32_t max_workers = 3;

      task_port () noexcept = default;

      task_port (const task_port&) = delete;
      task_port& operator= (const task_port&) = delete;

      ~task_port ();

      void
      open (dispatch_mode) noexcept;

      dispatch_mode
      mode () const noexcept
      {
        return mode_;
      }

      bool
      submit (std::uint32_t delay, task, void* context) noexcept;

      bool
      dispatch (std::uint32_t timeout) noexcept;

      void
      terminate () noexcept;

      void
      stop () noexcept;

      bool
      idle () const noexcept;

    private:
      struct item
      {
        task          run;
        void*         context;
        std::uint64_t due;
      };

      static void
      work (void*) noexcept;

      std::int32_t
      due () noexcept;

      std::uint64_t
      earliest () const noexcept;

      static constexpr std::uint32_t mask = capacity - 1;

      static_assert ((capacity & mask) == 0, "a power of two of task slots");

      dispatch_mode mode_ = dispatch_mode::manual;

      mutable mutex mutex_;
      condition     ready_;

      item          items_[capacity] {};
      std::uint32_t head_  = 0;
      std::uint32_t count_ = 0;

      bool terminated_ = false;
      bool canceled_   = false;

      std::atomic<std::uint32_t> queued_ {0};

      thread        workers_[max_workers];
      std::uint32_t worker_count_ = 0;
    };

    class task_queue
    {
    public:
      task_queue () noexcept = default;

      task_queue (const task_queue&) = delete;
      task_queue& operator= (const task_queue&) = delete;

      void
      open (task_port& w, task_port& c) noexcept
      {
        ports_[0] = &w;
        ports_[1] = &c;

        references_.store (1, std::memory_order_relaxed);
        terminated_.store (false, std::memory_order_relaxed);
      }

      task_port&
      operator[] (port p) noexcept
      {
        task_port* r (ports_[static_cast<std::uint32_t> (p) & 1]);

        LIBIW4X_PRE (r != nullptr);

        return *r;
      }

      unsigned
      duplicate () noexcept
      {
        return references_.fetch_add (1, std::memory_order_relaxed) + 1;
      }

      unsigned
      close () noexcept
      {
        return references_.fetch_sub (1, std::memory_order_acq_rel) - 1;
      }

      bool
      terminate () noexcept
      {
        return !terminated_.exchange (true, std::memory_order_acq_rel);
      }

    private:
      task_port*            ports_[2] {nullptr, nullptr};
      std::atomic<unsigned> references_ {1};
      std::atomic<bool>     terminated_ {false};
    };

    task_queue*
    create_queue (dispatch_mode work, dispatch_mode completion) noexcept;

    task_queue*
    create_composite_queue (task_port& work, task_port& completion) noexcept;

    void
    release_queue (task_queue&) noexcept;

    unsigned
    queue_count () noexcept;

    task_queue&
    process_queue () noexcept;

    void
    set_process_queue (task_queue*) noexcept;

    void
    stop_queues () noexcept;
  }
}
