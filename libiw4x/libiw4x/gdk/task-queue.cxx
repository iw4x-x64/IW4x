#include <libiw4x/gdk/task-queue.hxx>

#include <libiw4x/logger.hxx>

#include <libiw4x/gdk/error.hxx>

namespace iw4x
{
  namespace gdk
  {
    namespace
    {
      std::uint32_t
      worker_count (dispatch_mode m) noexcept
      {
        switch (m)
        {
        case dispatch_mode::async:
        case dispatch_mode::thread_pool:            return task_port::max_workers;
        case dispatch_mode::serialized_thread_pool: return 1;
        case dispatch_mode::manual:
        case dispatch_mode::immediate:              return 0;
        }

        return 0;
      }
    }

    task_port::
    ~task_port ()
    {
      stop ();
    }

    void task_port::
    open (dispatch_mode m) noexcept
    {
      mode_ = m;

      std::uint32_t n (worker_count (m));

      for (std::uint32_t i (0); i != n; ++i)
      {
        if (!workers_[i].start (&task_port::work, this))
        {
          warn ("unable to start a task queue worker");
          break;
        }

        ++worker_count_;
      }
    }

    void task_port::
    work (void* p) noexcept
    {
      task_port& t (*static_cast<task_port*> (p));

      while (t.dispatch (INFINITE))
        ;
    }

    bool task_port::
    submit (std::uint32_t delay, task run, void* context) noexcept
    {
      if (mode_ == dispatch_mode::immediate)
      {
        run (context, false);
        return true;
      }

      {
        lock l (mutex_);

        if (terminated_)
          return false;

        if (count_ == capacity)
        {
          warn ("a task queue port is full, {} callbacks outstanding", count_);
          return false;
        }

        items_[(head_ + count_) & mask] = item {run, context, now () + delay};

        ++count_;

        queued_.store (count_, std::memory_order_release);
      }

      ready_.wake_one ();
      return true;
    }

    std::int32_t task_port::
    due () noexcept
    {
      if (count_ == 0)
        return -1;

      if (terminated_)
        return 0;

      std::uint64_t t (now ());

      for (std::uint32_t i (0); i != count_; ++i)
      {
        if (items_[(head_ + i) & mask].due > t)
          continue;

        if (i != 0)
        {
          item x (items_[head_]);

          items_[head_]            = items_[(head_ + i) & mask];
          items_[(head_ + i) & mask] = x;
        }

        return 0;
      }

      return -1;
    }

    std::uint64_t task_port::
    earliest () const noexcept
    {
      std::uint64_t r (items_[head_].due);

      for (std::uint32_t i (1); i != count_; ++i)
      {
        std::uint64_t d (items_[(head_ + i) & mask].due);

        if (d < r)
          r = d;
      }

      return r;
    }

    bool task_port::
    dispatch (std::uint32_t timeout) noexcept
    {
      const bool infinite (timeout == INFINITE);

      if (timeout == 0 && queued_.load (std::memory_order_acquire) == 0)
        return false;

      lock l (mutex_);

      const std::uint64_t deadline (now () + (infinite ? 0 : timeout));

      for (;;)
      {
        if (due () == 0)
        {
          item i (items_[head_]);
          bool c (canceled_);

          head_ = (head_ + 1) & mask;
          --count_;

          queued_.store (count_, std::memory_order_release);

          l.release ();

          i.run (i.context, c);
          return true;
        }

        if (terminated_)
          return false;

        std::uint64_t t (now ());

        if (!infinite && t >= deadline)
          return false;

        DWORD w (infinite
                   ? INFINITE
                   : static_cast<DWORD> (deadline - t));

        if (count_ != 0)
        {
          std::uint64_t e (earliest ());

          if (e > t)
          {
            DWORD d (static_cast<DWORD> (e - t));

            if (infinite || d < w)
              w = d;
          }
          else
            w = 0;
        }

        ready_.wait (l, w);
      }
    }

    void task_port::
    terminate () noexcept
    {
      {
        lock l (mutex_);

        terminated_ = true;

        if (mode_ != dispatch_mode::manual)
          canceled_ = true;
      }

      ready_.wake_all ();
    }

    void task_port::
    stop () noexcept
    {
      {
        lock l (mutex_);

        terminated_ = true;
        canceled_   = true;
      }

      ready_.wake_all ();

      for (std::uint32_t i (0); i != worker_count_; ++i)
        workers_[i].join ();

      worker_count_ = 0;

      while (dispatch (0))
        ;
    }

    bool task_port::
    idle () const noexcept
    {
      lock l (mutex_);

      return count_ == 0;
    }

    namespace
    {
      constexpr std::uint32_t max_queues = 8;
      constexpr std::uint32_t max_ports  = 2 * max_queues;

      struct queue_pool
      {
        ~queue_pool ()
        {
          stop ();
        }

        void
        stop () noexcept
        {
          task_port* ps[max_ports];
          std::uint32_t n;

          {
            lock l (mutex_);

            n = ports;

            for (std::uint32_t i (0); i != n; ++i)
              ps[i] = &owned[i];
          }

          for (std::uint32_t i (0); i != n; ++i)
            ps[i]->stop ();
        }

        mutex         mutex_;
        task_port     owned[max_ports];
        task_queue    made[max_queues];
        std::uint32_t ports  = 0;
        std::uint32_t queues = 0;
      };

      queue_pool&
      pool () noexcept
      {
        static queue_pool p;
        return p;
      }

      std::atomic<task_queue*> current_process_queue {nullptr};
    }

    task_queue*
    create_queue (dispatch_mode w, dispatch_mode c) noexcept
    {
      queue_pool& p (pool ());
      lock        l (p.mutex_);

      if (p.queues == max_queues || p.ports + 2 > max_ports)
      {
        warn ("no room for another task queue, {} in use", p.queues);
        return nullptr;
      }

      task_port& wp (p.owned[p.ports++]);
      task_port& cp (p.owned[p.ports++]);

      wp.open (w);
      cp.open (c);

      task_queue& q (p.made[p.queues++]);

      q.open (wp, cp);

      return &q;
    }

    task_queue*
    create_composite_queue (task_port& w, task_port& c) noexcept
    {
      queue_pool& p (pool ());
      lock        l (p.mutex_);

      if (p.queues == max_queues)
      {
        warn ("no room for another task queue, {} in use", p.queues);
        return nullptr;
      }

      task_queue& q (p.made[p.queues++]);

      q.open (w, c);

      return &q;
    }

    task_queue&
    process_queue () noexcept
    {
      if (task_queue* q =
            current_process_queue.load (std::memory_order_acquire))
        return *q;

      static task_queue* fallback (
        [] () -> task_queue*
        {
          task_queue* q (create_queue (dispatch_mode::thread_pool,
                                       dispatch_mode::thread_pool));

          if (q == nullptr)
            fatal (chars ("unable to create the default process task queue"));

          info ("default process task queue created");
          return q;
        } ());

      task_queue* none (nullptr);

      current_process_queue.compare_exchange_strong (
        none,
        fallback,
        std::memory_order_acq_rel,
        std::memory_order_acquire);

      return *current_process_queue.load (std::memory_order_acquire);
    }

    void
    set_process_queue (task_queue* q) noexcept
    {
      current_process_queue.store (q, std::memory_order_release);

      info ("process task queue {}", q != nullptr ? "set" : "cleared");
    }

    void
    stop_queues () noexcept
    {
      pool ().stop ();
    }
  }
}
