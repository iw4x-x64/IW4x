#include <libiw4x/gdk/task-queue.hxx>

#include <libiw4x/gdk/async.hxx>

#include <new>

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
      {
        scope_lock l (mutex_);

        mode_       = m;
        head_       = 0;
        count_      = 0;
        terminated_ = false;
        canceled_   = false;

        queued_.store (0, std::memory_order_release);
      }

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
        scope_lock l (mutex_);

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

      scope_lock l (mutex_);

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
        scope_lock l (mutex_);

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
        scope_lock l (mutex_);

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
      scope_lock l (mutex_);

      return count_ == 0;
    }

    namespace
    {
      constexpr std::uint32_t max_queues = 64;
      constexpr std::uint32_t max_ports  = 32;

      struct queue_pool
      {
        ~queue_pool ()
        {
          stop ();
        }

        void
        stop () noexcept
        {
          task_port*    ps[max_ports];
          std::uint32_t n (0);

          {
            scope_lock l (mutex_);

            for (std::uint32_t i (0); i != max_ports; ++i)
            {
              if (port_used[i])
                ps[n++] = &owned[i];
            }
          }

          for (std::uint32_t i (0); i != n; ++i)
            ps[i]->stop ();
        }

        struct held_ports
        {
          std::uint32_t work       = max_ports;
          std::uint32_t completion = max_ports;
        };

        mutex         mutex_;
        task_port     owned[max_ports];
        task_queue    made[max_queues];
        bool          made_used[max_queues] {};
        bool          made_owns[max_queues] {};
        held_ports    made_ports[max_queues];
        bool          port_used[max_ports] {};
        bool          port_owned[max_ports] {};
        std::uint32_t port_borrows[max_ports] {};
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

      task_queue*
      take_queue (queue_pool& p) noexcept
      {
        for (std::uint32_t i (0); i != max_queues; ++i)
        {
          if (p.made_used[i])
            continue;

          p.made_used[i] = true;
          ++p.queues;

          return &p.made[i];
        }

        warn ("no room for another task queue, {} in use", p.queues);
        return nullptr;
      }

      std::uint32_t
      take_port (queue_pool& p) noexcept
      {
        for (std::uint32_t i (0); i != max_ports; ++i)
        {
          if (p.port_used[i])
            continue;

          p.port_used[i] = true;
          ++p.ports;

          return i;
        }

        return max_ports;
      }

      void
      give_port (queue_pool& p, std::uint32_t i) noexcept
      {
        if (i == max_ports)
          return;

        p.port_used[i] = false;
        --p.ports;
      }

      std::uint32_t
      port_index (queue_pool& p, task_port& t) noexcept
      {
        if (&t < p.owned || &t >= p.owned + max_ports)
          return max_ports;

        return static_cast<std::uint32_t> (&t - p.owned);
      }

      bool
      spent (queue_pool& p, std::uint32_t i) noexcept
      {
        return i != max_ports &&
               p.port_used[i] &&
               !p.port_owned[i] &&
               p.port_borrows[i] == 0;
      }
    }

    void
    release_queue (task_queue& q) noexcept
    {
      queue_pool& p (pool ());

      std::uint32_t done[2] {max_ports, max_ports};

      {
        scope_lock l (p.mutex_);

        for (std::uint32_t i (0); i != max_queues; ++i)
        {
          if (&p.made[i] != &q || !p.made_used[i])
            continue;

          queue_pool::held_ports h (p.made_ports[i]);

          std::uint32_t is[2] {h.work, h.completion};

          for (std::uint32_t k (0); k != 2; ++k)
          {
            std::uint32_t j (is[k]);

            if (j == max_ports)
              continue;

            if (p.made_owns[i])
              p.port_owned[j] = false;
            else if (p.port_borrows[j] != 0)
              --p.port_borrows[j];

            if (spent (p, j))
              done[k] = j;
          }

          p.made_ports[i] = queue_pool::held_ports ();
          p.made_owns[i]  = false;
          p.made_used[i]  = false;

          --p.queues;
          break;
        }
      }

      for (std::uint32_t j: done)
      {
        if (j != max_ports)
          p.owned[j].stop ();
      }

      if (done[0] == max_ports && done[1] == max_ports)
        return;

      scope_lock l (p.mutex_);

      for (std::uint32_t j: done)
        give_port (p, j);
    }

    unsigned
    queue_count () noexcept
    {
      queue_pool& p (pool ());
      scope_lock  l (p.mutex_);

      return p.queues;
    }

    task_queue*
    create_queue (dispatch_mode w, dispatch_mode c) noexcept
    {
      queue_pool& p (pool ());
      scope_lock  l (p.mutex_);

      std::uint32_t wi (take_port (p));
      std::uint32_t ci (take_port (p));

      task_queue* q (wi != max_ports && ci != max_ports ? take_queue (p)
                                                        : nullptr);

      if (q == nullptr)
      {
        if (wi == max_ports || ci == max_ports)
          warn ("no room for another task queue port pair, {} in use",
                p.ports);

        give_port (p, wi);
        give_port (p, ci);

        return nullptr;
      }

      std::uint32_t i (static_cast<std::uint32_t> (q - p.made));

      p.made_ports[i] = queue_pool::held_ports {wi, ci};
      p.made_owns[i]  = true;

      p.port_owned[wi] = true;
      p.port_owned[ci] = true;

      task_port& wp (p.owned[wi]);
      task_port& cp (p.owned[ci]);

      wp.open (w);
      cp.open (c);

      q->open (wp, cp);

      return q;
    }

    task_queue*
    create_composite_queue (task_port& w, task_port& c) noexcept
    {
      queue_pool& p (pool ());
      scope_lock  l (p.mutex_);

      task_queue* q (take_queue (p));

      if (q == nullptr)
        return nullptr;

      std::uint32_t i (static_cast<std::uint32_t> (q - p.made));
      std::uint32_t wi (port_index (p, w));
      std::uint32_t ci (port_index (p, c));

      p.made_ports[i] = queue_pool::held_ports {wi, ci};
      p.made_owns[i]  = false;

      if (wi != max_ports)
        ++p.port_borrows[wi];

      if (ci != max_ports)
        ++p.port_borrows[ci];

      q->open (w, c);

      return q;
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
    namespace
    {
      task_queue&
      resolve (task_queue* q) noexcept
      {
        return q != nullptr ? *q : process_queue ();
      }

      struct adapted
      {
        void  (*run) (void*, bool);
        void*   context;
      };

      void
      run_adapted (void* p, bool canceled) noexcept
      {
        auto* a (static_cast<adapted*> (p));

        a->run (a->context, canceled);

        delete a;
      }

      struct completion
      {
        void  (*run) (void*);
        void*   context;
      };

      void
      run_completion (void* p, bool) noexcept
      {
        auto* c (static_cast<completion*> (p));

        c->run (c->context);

        delete c;
      }

      bool
      submit_adapted (task_port& p,
                      std::uint32_t delay,
                      void* context,
                      void (*f) (void*, bool)) noexcept
      {
        auto* a (new (std::nothrow) adapted {f, context});

        if (a == nullptr)
          return false;

        if (p.submit (delay, &run_adapted, a))
          return true;

        delete a;
        return false;
      }
    }

    HRESULT WINAPI xasync::
    queue_create (void*,
                  dispatch_mode w,
                  dispatch_mode c,
                  task_queue** out) noexcept
    {
      return guard ("XTaskQueueCreate", [&] () -> HRESULT
      {
        if (out == nullptr)
          raise (E_POINTER, "no result pointer");

        task_queue* q (create_queue (w, c));

        if (q == nullptr)
          raise (E_OUTOFMEMORY, "no room for another task queue");

        info ("task queue created, work {} completion {}",
              static_cast<std::uint32_t> (w),
              static_cast<std::uint32_t> (c));

        *out = q;
        return S_OK;
      });
    }

    HRESULT WINAPI xasync::
    queue_create_composite (void*,
                            task_port* w,
                            task_port* c,
                            task_queue** out) noexcept
    {
      return guard ("XTaskQueueCreateComposite", [&] () -> HRESULT
      {
        if (out == nullptr || w == nullptr || c == nullptr)
          raise (E_POINTER, "no port or no result pointer");

        task_queue* q (create_composite_queue (*w, *c));

        if (q == nullptr)
          raise (E_OUTOFMEMORY, "no room for another task queue");

        *out = q;
        return S_OK;
      });
    }

    HRESULT WINAPI xasync::
    queue_get_port (void*, task_queue* q, port p, task_port** out) noexcept
    {
      return guard ("XTaskQueueGetPort", [&] () -> HRESULT
      {
        l2 ("XTaskQueueGetPort ({})", static_cast<std::uint32_t> (p));

        if (out == nullptr)
          raise (E_POINTER, "no result pointer");

        *out = &resolve (q)[p];
        return S_OK;
      });
    }

    HRESULT WINAPI xasync::
    queue_duplicate_handle (void*, task_queue* q, task_queue** out) noexcept
    {
      return guard ("XTaskQueueDuplicateHandle", [&] () -> HRESULT
      {
        if (q == nullptr || out == nullptr)
          raise (E_POINTER, "no queue or no result pointer");

        l1 ("task queue duplicated, {} references", q->duplicate ());

        *out = q;
        return S_OK;
      });
    }

    bool WINAPI xasync::
    queue_dispatch (void*, task_queue* q, port p, std::uint32_t t) noexcept
    {
      return guard ("XTaskQueueDispatch", false, [&] () -> bool
      {
        if (q == nullptr)
          return false;

        bool r ((*q)[p].dispatch (t));

        if (r)
          l1 ("XTaskQueueDispatch ({}) ran a callback",
              static_cast<std::uint32_t> (p));

        return r;
      });
    }

    HRESULT WINAPI xasync::
    queue_close_handle (void*, task_queue* q) noexcept
    {
      return guard ("XTaskQueueCloseHandle", [&] () -> HRESULT
      {
        if (q != nullptr)
        {
          unsigned n (q->close ());

          l1 ("task queue closed, {} references", n);

          if (n == 0)
            release_queue (*q);
        }

        return S_OK;
      });
    }

    HRESULT WINAPI xasync::
    queue_submit_callback (void*,
                           task_queue* q,
                           port p,
                           void* context,
                           void (*f) (void*, bool)) noexcept
    {
      return guard ("XTaskQueueSubmitCallback", [&] () -> HRESULT
      {
        if (q == nullptr || f == nullptr)
          raise_invalid ("no queue or no callback");

        l2 ("XTaskQueueSubmitCallback ({})", static_cast<std::uint32_t> (p));

        return submit_adapted ((*q)[p], 0, context, f) ? S_OK : E_ABORT;
      });
    }

    HRESULT WINAPI xasync::
    queue_submit_delayed_callback (void*,
                                   task_queue* q,
                                   port p,
                                   std::uint32_t delay,
                                   void* context,
                                   void (*f) (void*, bool)) noexcept
    {
      return guard ("XTaskQueueSubmitDelayedCallback", [&] () -> HRESULT
      {
        if (q == nullptr || f == nullptr)
          raise_invalid ("no queue or no callback");

        l2 ("XTaskQueueSubmitDelayedCallback ({}, {} ms)",
            static_cast<std::uint32_t> (p),
            delay);

        return submit_adapted ((*q)[p], delay, context, f) ? S_OK : E_ABORT;
      });
    }

    HRESULT WINAPI xasync::
    queue_terminate (void*,
                     task_queue* q,
                     bool wait,
                     void* context,
                     void (*f) (void*)) noexcept
    {
      return guard ("XTaskQueueTerminate", [&] () -> HRESULT
      {
        if (q == nullptr)
          raise_invalid ("no queue");

        if (!q->terminate ())
          return E_ABORT;

        info ("task queue terminating");

        (*q)[port::work].terminate ();

        if (f != nullptr)
        {
          auto* c (new (std::nothrow) completion {f, context});

          if (c == nullptr ||
              !(*q)[port::completion].submit (0, &run_completion, c))
          {
            delete c;

            f (context);
          }
        }

        (*q)[port::completion].terminate ();

        if (wait)
          while ((*q)[port::completion].dispatch (INFINITE))
            ;

        return S_OK;
      });
    }

    bool WINAPI xasync::
    queue_get_current_process (void*, task_queue** out) noexcept
    {
      return guard ("XTaskQueueGetCurrentProcessTaskQueue", false, [&] () -> bool
      {
        l2 ("XTaskQueueGetCurrentProcessTaskQueue");

        if (out == nullptr)
          return false;

        task_queue& q (process_queue ());

        q.duplicate ();

        *out = &q;
        return true;
      });
    }

    HRESULT WINAPI xasync::
    queue_set_current_process (void*, task_queue* q) noexcept
    {
      return guard ("XTaskQueueSetCurrentProcessTaskQueue", [&] () -> HRESULT
      {
        set_process_queue (q);
        return S_OK;
      });
    }
  }
}
