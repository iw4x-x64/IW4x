#include <libiw4x/gdk/async.hxx>

#include <atomic>
#include <new>
#include <utility>

#include <libiw4x/logger.hxx>
#include <libiw4x/contract.hxx>

#include <libiw4x/gdk/error.hxx>
#include <libiw4x/gdk/argument.hxx>

using namespace std;

namespace iw4x
{
  namespace gdk
  {
    operation::
    ~operation () = default;

    void operation::
    result (size_t, void*)
    {
    }

    event::
    ~event () = default;

    namespace
    {
      constexpr uintptr_t signature (0x4957345800000001ULL);

      constexpr unsigned hold_bits (16);
      constexpr uint64_t hold_mask ((uint64_t (1) << hold_bits) - 1);

      constexpr uintptr_t
      token_of (uint64_t s) noexcept
      {
        return static_cast<uintptr_t> (s >> hold_bits);
      }

      constexpr unsigned
      holds_of (uint64_t s) noexcept
      {
        return static_cast<unsigned> (s & hold_mask);
      }

      constexpr uint64_t
      slot_state (uintptr_t t, unsigned h) noexcept
      {
        LIBIW4X_PRE (h <= hold_mask);

        return (static_cast<uint64_t> (t) << hold_bits) | h;
      }

      constexpr size_t   cache_line (64);
      constexpr uint32_t max_operations (128);

      struct alignas (cache_line) async_state
      {
        atomic<uint64_t> state {0};
        atomic<bool>     completed {false};

        HRESULT status = pending;
        size_t  size = 0;

        alignas (cache_line) mutex mutex_;

        condition done;

        bool         retired = false;
        async_block* block = nullptr;
        task_queue*  queue = nullptr;
        const void*  id = nullptr;
        const char*  name = "operation";

        operation_ptr local;

        HRESULT (*provider) (uint32_t, const void*) = nullptr;
        void*     provider_context = nullptr;
      };

      static_assert (sizeof (atomic<uint64_t>) +
                     sizeof (atomic<bool>) +
                     sizeof (HRESULT) +
                     sizeof (size_t) <= cache_line,
                     "everything the poll path reads fits on one line");

      struct state_pool
      {
        mutex       mutex_;
        async_state slots[max_operations];
        uint32_t    free[max_operations];
        uint32_t    freed = 0;
        uint32_t    used = 0;
        uintptr_t   next = 1;
      };

      state_pool&
      states () noexcept
      {
        static state_pool p;
        return p;
      }

      void release (async_state*) noexcept;
      void dispose (async_state*) noexcept;

      task_queue&
      queue_of (async_block* a) noexcept
      {
        return a->queue != nullptr ? *a->queue : process_queue ();
      }

      async_state*
      acquire (async_block* a)
      {
        LIBIW4X_PRE (a != nullptr);

        state_pool& s (states ());

        async_state* r (nullptr);

        uintptr_t t (0);

        {
          scope_lock l (s.mutex_);

          if (s.freed != 0)
            r = &s.slots[s.free[--s.freed]];
          else if (s.used != max_operations)
            r = &s.slots[s.used++];
          else
            raise (E_OUTOFMEMORY,
                   "no room for another operation, {} in flight",
                   max_operations);

          t = s.next++;
        }

        {
          scope_lock l (r->mutex_);

          r->completed.store (false, memory_order_relaxed);
          r->retired = false;
          r->block = a;
          r->status = pending;
          r->size = 0;
          r->provider = nullptr;
          r->provider_context = nullptr;

          a->internal[0] = reinterpret_cast<void*> (signature);
          a->internal[1] = r;
          a->internal[2] = reinterpret_cast<void*> (t);
          a->internal[3] = nullptr;

          r->state.store (slot_state (t, 1), memory_order_release);
        }

        return r;
      }

      class held_state
      {
      public:
        explicit
        held_state (async_block* a) noexcept: state_ (find (a)) {}

        ~held_state ()
        {
          if (state_ != nullptr)
            release (state_);
        }

        held_state (const held_state&) = delete;
        held_state& operator= (const held_state&) = delete;

        explicit
        operator bool () const noexcept
        {
          return state_ != nullptr;
        }

        async_state*
        operator-> () const noexcept
        {
          LIBIW4X_PRE (state_ != nullptr);

          return state_;
        }

        async_state&
        operator* () const noexcept
        {
          LIBIW4X_PRE (state_ != nullptr);

          return *state_;
        }

        async_state*
        get () const noexcept
        {
          return state_;
        }

      private:
        static async_state*
        find (async_block* a) noexcept
        {
          if (a == nullptr ||
              a->internal[0] != reinterpret_cast<void*> (signature))
            return nullptr;

          auto* r (static_cast<async_state*> (a->internal[1]));

          if (r == nullptr)
            return nullptr;

          uintptr_t t (reinterpret_cast<uintptr_t> (a->internal[2]));

          if (t == 0)
            return nullptr;

          uint64_t s (r->state.load (memory_order_acquire));

          for (;;)
          {
            if (token_of (s) != t)
              return nullptr;

            if (r->state.compare_exchange_weak (s,
                                                s + 1,
                                                memory_order_acq_rel,
                                                memory_order_acquire))
              return r;
          }
        }

        async_state* state_;
      };

      void
      release (async_state* r) noexcept
      {
        const uint64_t s (r->state.fetch_sub (1, memory_order_acq_rel));

        LIBIW4X_ASSERT (holds_of (s) != 0);

        if (holds_of (s) == 1)
          dispose (r);
      }

      enum class provider_op: uint32_t
      {
        begin      = 0,
        do_work    = 1,
        get_result = 2,
        cancel     = 3,
        cleanup    = 4
      };

      struct provider_data
      {
        async_block* async;
        size_t       buffer_size;
        void*        buffer;
        void*        context;
      };

      HRESULT
      call_provider (async_state* r,
                     provider_op op,
                     size_t size,
                     void* buffer) noexcept
      {
        LIBIW4X_PRE (r->provider != nullptr);

        provider_data d {r->block, size, buffer, r->provider_context};

        return r->provider (static_cast<uint32_t> (op), &d);
      }

      void
      dispose (async_state* r) noexcept
      {
        HRESULT (*p) (uint32_t, const void*) (nullptr);

        void*         c (nullptr);
        operation_ptr o;

        {
          scope_lock l (r->mutex_);

          p = r->provider;
          c = r->provider_context;
          o = move (r->local);

          r->provider = nullptr;
          r->provider_context = nullptr;
        }

        o.reset ();

        if (p != nullptr)
        {
          provider_data d {nullptr, 0, nullptr, c};

          p (static_cast<uint32_t> (provider_op::cleanup), &d);
        }

        state_pool& s (states ());
        scope_lock  l (s.mutex_);

        LIBIW4X_ASSERT (r >= s.slots && r < s.slots + max_operations);
        LIBIW4X_ASSERT (s.freed < max_operations);

        r->state.store (0, memory_order_release);

        s.free[s.freed++] = static_cast<uint32_t> (r - s.slots);
      }

      void
      retire (async_state* r) noexcept
      {
        bool drop (false);

        {
          scope_lock l (r->mutex_);

          if (!r->retired)
          {
            r->retired = true;
            drop = true;

            if (r->block != nullptr)
            {
              r->block->internal[0] = nullptr;
              r->block->internal[1] = nullptr;
              r->block->internal[2] = nullptr;
              r->block = nullptr;
            }
          }
        }

        if (drop)
          release (r);
      }

      struct held_task
      {
        async_state* state;
        void       (*run) (async_state*, bool);
      };

      void
      run_held (void* p, bool canceled) noexcept
      {
        auto* t (static_cast<held_task*> (p));

        LIBIW4X_PRE (t != nullptr);

        async_state* r (t->state);

        t->run (r, canceled);

        release (r);
      }

      bool
      submit (async_state* r,
              task_port& p,
              uint32_t delay,
              void (*f) (async_state*, bool)) noexcept
      {
        const uint64_t s (r->state.fetch_add (1, memory_order_acq_rel));

        LIBIW4X_ASSERT (holds_of (s) != 0 && holds_of (s) != hold_mask);

        auto* t (new (nothrow) held_task {r, f});

        if (t != nullptr && p.submit (delay, &run_held, t))
          return true;

        delete t;

        release (r);
        return false;
      }

      void
      announce (async_state* r, bool) noexcept
      {
        async_block* a;
        void       (*f) (async_block*);

        {
          scope_lock l (r->mutex_);

          a = r->block;
          f = a != nullptr ? a->callback : nullptr;
        }

        if (f != nullptr)
          f (a);
      }

      void
      finish (async_state* r, HRESULT hr, size_t size) noexcept
      {
        async_block* a;

        {
          scope_lock l (r->mutex_);

          if (r->completed.load (memory_order_relaxed))
            return;

          r->status = hr;
          r->size = size;

          r->completed.store (true, memory_order_release);

          a = r->block;
        }

        l1 ("{} completed {:#010x}", r->name, static_cast<uint32_t> (hr));

        r->done.wake_all ();

        if (a == nullptr || a->callback == nullptr)
          return;

        if (!submit (r, queue_of (a)[port::completion], 0, &announce))
          warn ("{}: the queue would not take its completion routine",
                r->name);
      }

      void
      perform (async_state* r, bool canceled) noexcept
      {
        l1 ("{} work {}", r->name, canceled ? "canceled" : "running");

        if (canceled)
        {
          finish (r, E_ABORT, 0);
          return;
        }

        LIBIW4X_PRE (r->local != nullptr);

        HRESULT hr (S_OK);
        size_t  size (0);

        try
        {
          size = r->local->work ();
        }
        catch (...)
        {
          hr = report (r->name);
        }

        finish (r, hr, SUCCEEDED (hr) ? size : 0);
      }

      void
      do_work (async_state* r, bool canceled) noexcept
      {
        if (canceled)
        {
          call_provider (r, provider_op::cancel, 0, nullptr);
          finish (r, E_ABORT, 0);
          return;
        }

        HRESULT hr (call_provider (r, provider_op::do_work, 0, nullptr));

        if (hr != pending && hr != S_OK)
          finish (r, hr, 0);
      }

      HRESULT
      take_result (async_block* a,
                   const void* id,
                   size_t size,
                   void* buffer,
                   size_t* used) noexcept
      {
        held_state r (a);

        if (!r)
          return E_INVALIDARG;

        if (!r->completed.load (memory_order_acquire))
          return pending;

        HRESULT hr (r->status);

        if (SUCCEEDED (hr) && id != nullptr && r->id != id)
          return E_INVALIDARG;

        if (SUCCEEDED (hr))
        {
          try
          {
            if (r->local)
              r->local->result (size, buffer);
            else if (r->provider != nullptr)
              hr = call_provider (r.get (),
                                  provider_op::get_result,
                                  size,
                                  buffer);
          }
          catch (...)
          {
            hr = report (r->name);
          }
        }

        if (SUCCEEDED (hr) && used != nullptr)
          *used = r->size;

        retire (r.get ());
        return hr;
      }

      struct posted
      {
        event_ptr what;
      };

      void
      deliver (void* p, bool canceled) noexcept
      {
        auto* e (static_cast<posted*> (p));

        LIBIW4X_PRE (e != nullptr && e->what != nullptr);

        if (!canceled)
        {
          try
          {
            e->what->deliver ();
          }
          catch (...)
          {
            report ("posted event");
          }
        }

        delete e;
      }
    }

    void
    begin (async_block* a, const operation_id& id, operation_ptr o)
    {
      if (a == nullptr)
        raise (E_POINTER, "no async block");

      LIBIW4X_PRE (o != nullptr);
      LIBIW4X_PRE (id.name != nullptr);

      task_queue& q (queue_of (a));

      async_state* r (acquire (a));

      r->queue = &q;
      r->id = id.token ();
      r->name = id.name;
      r->local = move (o);

      l1 ("{} begun", id.name);

      if (!submit (r, q[port::work], 0, &perform))
      {
        retire (r);

        raise (E_ABORT, "{}: the task queue is going down", id.name);
      }
    }

    HRESULT
    result (async_block* a,
            const operation_id& id,
            size_t size,
            void* buffer) noexcept
    {
      return take_result (a, id.token (), size, buffer, nullptr);
    }

    HRESULT
    result_size (async_block* a, size_t* out) noexcept
    {
      held_state r (a);

      if (!r || out == nullptr)
        return E_INVALIDARG;

      if (!r->completed.load (memory_order_acquire))
        return pending;

      *out = r->size;
      return r->status;
    }

    operation*
    pending_operation (async_block* a, const operation_id& id) noexcept
    {
      held_state r (a);

      if (!r || r->id != id.token ())
        return nullptr;

      return r->local.get ();
    }

    bool
    post (task_queue* q, event_ptr e) noexcept
    {
      if (!e)
        return false;

      task_queue& t (q != nullptr ? *q : process_queue ());

      auto* p (new (nothrow) posted {move (e)});

      if (p == nullptr)
        return false;

      if (t[port::completion].submit (0, &deliver, p))
        return true;

      delete p;
      return false;
    }

    HRESULT WINAPI xasync::
    get_status (void*, async_block* a, bool wait) noexcept
    {
      return guard ("XAsyncGetStatus", [&] () -> HRESULT
      {
        l3 ("XAsyncGetStatus (wait {})", wait);

        held_state r (a);

        if (!r)
          raise_invalid ("not an operation of ours");

        if (!wait)
          return r->completed.load (memory_order_acquire) ? r->status
                                                               : pending;

        scope_lock l (r->mutex_);

        while (!r->completed.load (memory_order_relaxed))
          r->done.wait (l, INFINITE);

        return r->status;
      });
    }

    HRESULT WINAPI xasync::
    get_result_size (void*, async_block* a, size_t* out) noexcept
    {
      return guard ("XAsyncGetResultSize", [&] () -> HRESULT
      {
        l2 ("XAsyncGetResultSize");

        return result_size (a, out);
      });
    }

    HRESULT WINAPI xasync::
    get_result (void*,
                async_block* a,
                const void* id,
                size_t size,
                void* buffer,
                size_t* used) noexcept
    {
      return guard ("XAsyncGetResult", [&] () -> HRESULT
      {
        HRESULT hr (take_result (a, id, size, buffer, used));

        l2 ("XAsyncGetResult (size {}) used {}",
            size,
            used != nullptr ? *used : 0);

        return hr;
      });
    }

    HRESULT WINAPI xasync::
    cancel (void*, async_block* a) noexcept
    {
      return guard ("XAsyncCancel", [&] () -> HRESULT
      {
        l2 ("XAsyncCancel");

        held_state r (a);

        if (!r)
          return S_OK;

        if (!r->completed.load (memory_order_acquire))
          finish (r.get (), E_ABORT, 0);

        retire (r.get ());
        return S_OK;
      });
    }

    HRESULT WINAPI xasync::
    provider_begin (void*,
                    async_block* a,
                    void* context,
                    const void* id,
                    const char* n,
                    HRESULT (*provider) (uint32_t,
                                         const void*)) noexcept
    {
      return guard ("XAsyncBegin", [&] () -> HRESULT
      {
        if (a == nullptr || provider == nullptr)
          raise_invalid ("no async block or no provider");

        async_state* r (acquire (a));

        r->queue = &queue_of (a);
        r->id = id;
        r->name = n != nullptr ? n : "operation";
        r->provider = provider;
        r->provider_context = context;

        l1 ("XAsyncBegin ({})", r->name);

        HRESULT hr (call_provider (r, provider_op::begin, 0, nullptr));

        if (FAILED (hr))
        {
          call_provider (r, provider_op::cleanup, 0, nullptr);
          retire (r);
        }

        return hr;
      });
    }

    HRESULT WINAPI xasync::
    schedule (void*, async_block* a, uint32_t delay) noexcept
    {
      return guard ("XAsyncSchedule", [&] () -> HRESULT
      {
        l2 ("XAsyncSchedule (delay {} ms)", delay);

        held_state r (a);

        if (!r)
          raise_invalid ("not an operation of ours");

        if (r->provider == nullptr)
          raise_invalid ("not an operation with a provider");

        LIBIW4X_ASSERT (r->queue != nullptr);

        return submit (r.get (), (*r->queue)[port::work], delay, &do_work)
                 ? S_OK
                 : E_ABORT;
      });
    }

    HRESULT WINAPI xasync::
    complete (void*, async_block* a, HRESULT hr, size_t size) noexcept
    {
      return guard ("XAsyncComplete", [&] () -> HRESULT
      {
        l1 ("XAsyncComplete ({:#010x}, {} bytes)",
            static_cast<uint32_t> (hr),
            size);

        held_state r (a);

        if (!r)
          raise_invalid ("not an operation of ours");

        finish (r.get (), hr, size);
        return S_OK;
      });
    }

    HRESULT WINAPI xasync::
    precall (void*) noexcept
    {
      return S_OK;
    }
  }
}
