#pragma once

#include <libiw4x/logger.hxx>

#include <libiw4x/export.hxx>

namespace iw4x
{
  // Domain tag.
  //
  struct com_frame_domain {};

  // Callable type alias.
  //
  using task = move_only_function<void ()>;

  // Scheduling modes.
  //
  // Each mode is a distinct type encoding the scheduling behavior at compile
  // time. The idea is to avoid runtime flags in the hot path. Instead, overload
  // resolution selects the correct enqueue mechanism (for example, atomic queue
  // versus local vector) based on the mode type alone.
  //

  // Execute the task on the next tick of the target scheduler.
  //
  // This is the default behavior when no specific mode argument is provided.
  //
  struct immediate {};

  // Execute on the next tick, but force the task through the atomic
  // cross-thread ingress queue.
  //
  // This mode must be used when posting from a thread that does not own the
  // target scheduler (foreign thread). It is also safe, though less efficient,
  // to use from the owning thread.
  //
  struct asynchronous {};

  // Execute on every tick indefinitely.
  //
  // The task is re-enqueued after each execution and persists until the
  // scheduler is destroyed.
  //
  struct repeat_every_tick {};

  // Scheduled entry.
  //
  // Internal representation of a unit of work. All scheduling modes are
  // unified into this single layout to allow the pending and active queues
  // to store heterogeneous entries.
  //
  // Note that this type is rarely interacted with directly. It is populated
  // implicitly by logical_scheduler::post().
  //
  struct scheduled_entry
  {
    task work;

    // Retention policy.
    //
    // Called after execution to determine whether this entry should survive to
    // the next tick.
    //
    bool (*retain) (scheduled_entry&) = nullptr;

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
  // The model is explicit ownership. Each scheduler maintains its own task
  // queues and must be ticked explicitly by its owner. There are no implicit
  // wakeups or cross-scheduler interactions, except via the asynchronous
  // ingress queue.
  //
  // Note that ownership is claimed implicitly on the first call to tick() and
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
    // pending buffer. It must only be called from the thread that owns this
    // scheduler.
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

    // Schedule work to be executed on every tick.
    //
    void
    post (task work, repeat_every_tick mode);

    // Execution.
    //

    // Execute all pending tasks for the current tick.
    //
    // The execution logic is double-buffered. We first drain the async ingress
    // queue into the pending buffer and then swap the pending buffer with the
    // active one. Once swapped, we iterate over the active snapshot
    // front-to-back.
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
    // allocation is isolated to the async path. Same-thread posts remain
    // allocation-free (amortized).
    //
    struct async_node
    {
      async_node* next;
      scheduled_entry entry;
    };

    // Drain all nodes from the async ingress queue into the local pending
    // buffer.
    //
    // The drained list is LIFO, so we reverse it to restore FIFO ordering
    // before appending.
    //
    void
    drain_async ();

    // Head of the ingress queue.
    //
    // Foreign threads push here via CAS. The owning thread consumes the entire
    // list via atomic exchange.
    //
    atomic<async_node*> async_head_;

    // Local queues.
    //

    // Double-buffered local queues.
    //
    // The pending_ queue accumulates tasks between ticks and during the current
    // tick. The active_ queue holds the snapshot being processed. We swap them
    // at the start of the tick to keep the iteration range stable.
    //
    vector<scheduled_entry> pending_;
    vector<scheduled_entry> active_;

    // Diagnostics.
    //

    // Identity of the thread that first ticked this scheduler.
    //
    // Used to detect accidental cross-thread access to non-thread-safe methods.
    //
    jthread::id owner_;
  };

  // Global registry.
  //
  // Provides a unified interface for locating schedulers based on domain tags.
  //
  // We map domain tag types (for example, com_frame_domain) to
  // logical_scheduler instances using template-parameterized static locals. The
  // idea is to avoid dynamic map lookup by making each domain resolve to
  // exactly one scheduler instance at link time.
  //
  namespace scheduler
  {
    // Retrieve the logical scheduler instance for the specified domain.
    //
    // The instance is created on first access.
    //
    template <typename Domain>
    logical_scheduler&
    get ()
    {
      static logical_scheduler s;
      return s;
    }

    // Dispatch a standard task to the domain-specific scheduler.
    //
    template <typename Domain>
    void
    post (Domain, task work)
    {
      get<Domain> ().post (static_cast<task&&> (work));
    }

    // Dispatch an asynchronous task to the domain-specific scheduler.
    //
    template <typename Domain>
    void
    post (Domain, task work, asynchronous mode)
    {
      get<Domain> ().post (static_cast<task&&> (work), mode);
    }

    // Dispatch a repeating task to the domain-specific scheduler.
    //
    template <typename Domain>
    void
    post (Domain, task work, repeat_every_tick mode)
    {
      get<Domain> ().post (static_cast<task&&> (work), mode);
    }
  }
}
