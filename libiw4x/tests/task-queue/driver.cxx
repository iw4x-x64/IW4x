#undef NDEBUG
#include <cassert>

#include <atomic>

#include <libiw4x/gdk/sync.hxx>
#include <libiw4x/gdk/task-queue.hxx>

using namespace iw4x::gdk;

namespace
{
  std::atomic<int> ran {0};
  std::atomic<int> canceled {0};

  void
  count (void*, bool c) noexcept
  {
    if (c)
      canceled.fetch_add (1, std::memory_order_relaxed);
    else
      ran.fetch_add (1, std::memory_order_relaxed);
  }

  void
  reset () noexcept
  {
    ran.store (0, std::memory_order_relaxed);
    canceled.store (0, std::memory_order_relaxed);
  }
}

int
main ()
{
  {
    task_port p;
    p.open (dispatch_mode::manual);

    reset ();

    assert (p.idle ());
    assert (!p.dispatch (0));

    assert (p.submit (0, &count, nullptr));
    assert (!p.idle ());

    assert (p.dispatch (0));
    assert (ran.load () == 1);

    assert (!p.dispatch (0));
    assert (p.idle ());
  }

  {
    task_port p;
    p.open (dispatch_mode::manual);

    reset ();

    assert (p.submit (200, &count, nullptr));
    assert (!p.dispatch (0));
    assert (ran.load () == 0);

    assert (p.dispatch (INFINITE));
    assert (ran.load () == 1);
  }

  {
    task_port p;
    p.open (dispatch_mode::manual);

    reset ();

    assert (p.submit (0, &count, nullptr));
    assert (p.submit (0, &count, nullptr));

    p.terminate ();

    assert (!p.submit (0, &count, nullptr));

    assert (p.dispatch (INFINITE));
    assert (p.dispatch (INFINITE));
    assert (!p.dispatch (INFINITE));

    assert (ran.load () == 2);
    assert (canceled.load () == 0);
  }

  {
    task_port p;
    p.open (dispatch_mode::manual);

    reset ();

    assert (p.submit (60000, &count, nullptr));

    p.stop ();

    assert (canceled.load () == 1);
    assert (ran.load () == 0);
  }

  {
    task_port p;
    p.open (dispatch_mode::immediate);

    reset ();

    assert (p.submit (0, &count, nullptr));
    assert (ran.load () == 1);
    assert (p.idle ());
  }

  {
    task_port p;
    p.open (dispatch_mode::thread_pool);

    reset ();

    for (int i (0); i != 50; ++i)
      assert (p.submit (0, &count, nullptr));

    p.stop ();

    assert (ran.load () + canceled.load () == 50);
  }

  {
    task_port p;
    p.open (dispatch_mode::manual);

    reset ();

    for (std::uint32_t i (0); i != task_port::capacity; ++i)
      assert (p.submit (60000, &count, nullptr));

    assert (!p.submit (60000, &count, nullptr));

    p.stop ();

    assert (canceled.load () == static_cast<int> (task_port::capacity));
  }

  {
    task_queue* q (create_queue (dispatch_mode::async,
                                 dispatch_mode::manual));

    assert (q != nullptr);
    assert ((*q)[port::work].mode () == dispatch_mode::async);
    assert ((*q)[port::completion].mode () == dispatch_mode::manual);

    assert (q->duplicate () == 2);
    assert (q->close () == 1);

    assert (q->terminate ());
    assert (!q->terminate ());
  }

  {
    task_queue& q (process_queue ());

    assert (&process_queue () == &q);

    task_queue* o (create_queue (dispatch_mode::manual,
                                 dispatch_mode::manual));

    assert (o != nullptr);

    set_process_queue (o);

    assert (&process_queue () == o);

    set_process_queue (&q);
  }

  stop_queues ();
}
