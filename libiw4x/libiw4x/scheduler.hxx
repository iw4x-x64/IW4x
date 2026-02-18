#pragma once

#include <libiw4x/export.hxx>

namespace iw4x
{
  struct com_frame_domain {};

  // Callable type aliases.
  //

  using task = move_only_function<void ()>;

  // Scheduling modes.
  //
  // Each mode is a distinct type encoding the scheduling behavior at compile
  // time. The idea is to avoid runtime flags in the hot path. Instead, we rely
  // on overload resolution to select the correct enqueue mechanism (e.g.,
  // atomic queue vs local vector) based on the mode type alone.
  //

  // Execute the task on the next tick of the target scheduler. This is the
  // default behavior when no specific mode argument is provided.
  //
  struct immediate {};

  // Execute on the next tick, but force the task through the atomic cross-
  // thread ingress queue.
  //
  // This mode must be used when posting from a thread that does not own
  // the target scheduler (foreign thread). It is also safe, though less
  // efficient, to use from the owning thread.
  //
  struct asynchronous {};

  // Scheduled entry.
  //
  // Internal representation of a unit of work. We unify all scheduling modes
  // into this single layout to allow the pending and active queues to store
  // heterogeneous entries.
  //
  // Note that we should rarely, if ever, interact with this type directly. It's
  // populated implicitly by the logical_scheduler::post() logic (see below).
  //
  struct scheduled_entry
  {
    task work;

    scheduled_entry () = default;
    scheduled_entry (scheduled_entry&&) = default;
    scheduled_entry& operator= (scheduled_entry&&) = default;

    scheduled_entry (const scheduled_entry&) = delete;
    scheduled_entry& operator= (const scheduled_entry&) = delete;
  };

  // Logical scheduler.
  //
  // An independently tickable scheduler bound to a single owning thread.
  //
  // The model here is explicit ownership: each scheduler maintains its own task
  // queues and must be ticked explicitly by its owner. We do not provide
  // implicit wakeups and/or cross-scheduler interaction (except via the async
  // ingress queue).
  //
  // Note that wwnership is claimed implicitly on the first call to tick() and
  // asserted on every subsequent call.
  //
  class LIBIW4X_SYMEXPORT logical_scheduler
  {
  public:
    logical_scheduler ();
    ~logical_scheduler ();

    logical_scheduler (const logical_scheduler&) = delete;
    logical_scheduler& operator= (const logical_scheduler&) = delete;

    logical_scheduler (logical_scheduler&&) = delete;
    logical_scheduler& operator= (logical_scheduler&&) = delete;

    // Posting.
    //

    // Schedule work to be executed on the next tick.
    //
    // This overload bypasses the atomic queue and appends directly to the local
    // pending buffer. In other words, it must only be called from the thread
    // that owns this scheduler.
    //
    void
    post (task work);

    // Schedule work from a foreign thread.
    //
    // Pushes the task onto the atomic ingress queue. This is safe to call from
    // any thread. The task will be drained into the local pending queue at the
    // start of the next tick.
    //
    void
    post (task work, asynchronous mode);

    // Execution.
    //

    // Execute all pending tasks for the current tick.
    //
    // The execution logic is double-buffered: we first drain the async
    // ingress queue into the pending buffer and then swap the pending
    // buffer with the active one. Once swapped, we iterate over the
    // active snapshot front-to-back.
    //
    // This separation ensures that tasks posted *during* execution (e.g.,
    // by the work itself) land in the (now empty) pending buffer and are
    // deferred to the *next* tick. This prevents unbounded recursion
    // within a single frame.
    //
    void
    tick ();

  private:
    // Ingress queue details.
    //

    // Node for the lock-free MPSC ingress queue.
    //
    // Each cross-thread post allocates one node. The owning thread then adopts
    // and deallocates these nodes during the drain phase. Note that this
    // allocation is isolated to the async path. That is, same-thread posts
    // remain allocation-free (amortized).
    //
    struct async_node
    {
      async_node* next;
      scheduled_entry entry;
    };

    // Drain all nodes from the async ingress queue into the local pending
    // buffer. The drained list is LIFO, so we reverse it to restore FIFO
    // ordering before appending.
    //
    void
    drain_async ();

    // The head of the ingress queue. Foreign threads push here via CAS. That
    // is, the owning thread consumes the entire list via atomic exchange.
    //
    atomic<async_node*> async_head_;

    // Local queues.
    //

    // Double-buffered local queues.
    //
    // pending_ accumulates tasks between ticks (and during the current
    // tick). active_ holds the snapshot being processed. We swap them at
    // the start of the tick to keep the iteration range stable.
    //
    vector<scheduled_entry> pending_;
    vector<scheduled_entry> active_;

    // Diagnostics.
    //

    // The identity of the thread that first ticked this scheduler. Used to
    // detect accidental cross-thread access to non-thread-safe methods.
    //
    jthread::id owner_;
  };

  // Global registry.
  //
  // This namespace provides a unified interface for locating schedulers based
  // on domain tags.
  //
  // We map domain tag types (e.g., frame_domain) to logical_scheduler instances
  // using template-parameterized static locals. The general idea is to avoid
  // dynamic map lookup by making each domain resolves to exactly one scheduler
  // instance at link time.
  //
  namespace scheduler
  {
    // Retrieve the logical scheduler instance for the specified Domain.
    // The instance is created on first access .
    //
    template <typename Domain>
    logical_scheduler&
    get ()
    {
      static logical_scheduler s;
      return s;
    }

    template <typename Domain>
    void
    post (Domain, task work)
    {
      get<Domain> ().post (static_cast<task&&> (work));
    }

    template <typename Domain>
    void
    post (Domain, task work, asynchronous mode)
    {
      get<Domain> ().post (static_cast<task&&> (work), mode);
    }
  }
}
