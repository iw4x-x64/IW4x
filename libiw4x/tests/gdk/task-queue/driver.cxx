#undef NDEBUG
#include <cassert>

#include <atomic>

#include <libiw4x/gdk/sync.hxx>
#include <libiw4x/gdk/async.hxx>
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
  bare (void*) noexcept
  {
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

  {
    task_queue* q (nullptr);

    assert (xasync::queue_create (nullptr,
                                  dispatch_mode::manual,
                                  dispatch_mode::manual,
                                  &q) == S_OK);
    assert (q != nullptr);

    assert (xasync::queue_create (nullptr,
                                  dispatch_mode::manual,
                                  dispatch_mode::manual,
                                  nullptr) == E_POINTER);

    task_port* w (nullptr);
    task_port* c (nullptr);

    assert (xasync::queue_get_port (nullptr, q, port::work, &w) == S_OK);
    assert (xasync::queue_get_port (nullptr, q, port::completion, &c) == S_OK);
    assert (w == &(*q)[port::work] && c == &(*q)[port::completion]);

    task_queue* composite (nullptr);

    assert (xasync::queue_create_composite (nullptr, w, c, &composite) ==
            S_OK);
    assert (composite != nullptr);
    assert (&(*composite)[port::work] == w);

    assert (xasync::queue_create_composite (nullptr, nullptr, c, &composite) ==
            E_POINTER);

    task_queue* d (nullptr);

    assert (xasync::queue_duplicate_handle (nullptr, q, &d) == S_OK);
    assert (d == q);

    assert (xasync::queue_duplicate_handle (nullptr, nullptr, &d) ==
            E_POINTER);

    reset ();

    assert (xasync::queue_submit_callback (nullptr,
                                           q,
                                           port::work,
                                           nullptr,
                                           &count) == S_OK);

    assert (xasync::queue_dispatch (nullptr, q, port::work, 0));
    assert (ran.load () == 1);

    assert (!xasync::queue_dispatch (nullptr, q, port::work, 0));
    assert (!xasync::queue_dispatch (nullptr, nullptr, port::work, 0));

    assert (xasync::queue_submit_delayed_callback (nullptr,
                                                   q,
                                                   port::work,
                                                   1,
                                                   nullptr,
                                                   &count) == S_OK);

    assert (xasync::queue_dispatch (nullptr, q, port::work, INFINITE));
    assert (ran.load () == 2);

    assert (xasync::queue_submit_callback (nullptr,
                                           q,
                                           port::work,
                                           nullptr,
                                           nullptr) == E_INVALIDARG);

    assert (xasync::queue_submit_delayed_callback (nullptr,
                                                   nullptr,
                                                   port::work,
                                                   0,
                                                   nullptr,
                                                   &count) == E_INVALIDARG);

    reset ();

    assert (xasync::queue_terminate (nullptr, q, false, nullptr, &bare) ==
            S_OK);

    assert (xasync::queue_terminate (nullptr, q, false, nullptr, &bare) ==
            E_ABORT);

    assert (xasync::queue_terminate (nullptr, nullptr, false, nullptr,
                                     nullptr) == E_INVALIDARG);

    while (xasync::queue_dispatch (nullptr, q, port::completion, 0))
      ;

    assert (ran.load () == 1);

    assert (xasync::queue_close_handle (nullptr, q) == S_OK);
    assert (xasync::queue_close_handle (nullptr, nullptr) == S_OK);
  }

  {
    task_queue* q (nullptr);

    assert (xasync::queue_get_current_process (nullptr, &q));
    assert (q == &process_queue ());

    assert (!xasync::queue_get_current_process (nullptr, nullptr));

    assert (xasync::queue_set_current_process (nullptr, q) == S_OK);
    assert (&process_queue () == q);
  }

  stop_queues ();

  {
    unsigned before (queue_count ());

    for (unsigned i (0); i != 128; ++i)
    {
      task_queue* q (nullptr);

      assert (xasync::queue_create (nullptr,
                                    dispatch_mode::manual,
                                    dispatch_mode::manual,
                                    &q) == S_OK);
      assert (q != nullptr);

      task_port* w (nullptr);
      task_port* c (nullptr);

      assert (xasync::queue_get_port (nullptr, q, port::work, &w) == S_OK);
      assert (xasync::queue_get_port (nullptr, q, port::completion, &c) ==
              S_OK);

      task_queue* composite (nullptr);

      assert (xasync::queue_create_composite (nullptr, w, c, &composite) ==
              S_OK);

      reset ();

      assert (xasync::queue_submit_callback (nullptr,
                                             q,
                                             port::work,
                                             nullptr,
                                             &count) == S_OK);

      assert (xasync::queue_dispatch (nullptr, q, port::work, 0));
      assert (ran.load (std::memory_order_relaxed) == 1);

      assert (xasync::queue_close_handle (nullptr, composite) == S_OK);
      assert (xasync::queue_close_handle (nullptr, q) == S_OK);
    }

    assert (queue_count () == before);
  }

  {
    task_queue* q (nullptr);

    assert (xasync::queue_create (nullptr,
                                  dispatch_mode::manual,
                                  dispatch_mode::manual,
                                  &q) == S_OK);

    task_port* w (nullptr);

    assert (xasync::queue_get_port (nullptr, q, port::work, &w) == S_OK);

    task_queue* composite (nullptr);

    assert (xasync::queue_create_composite (nullptr, w, w, &composite) ==
            S_OK);

    assert (xasync::queue_close_handle (nullptr, q) == S_OK);

    reset ();

    assert (xasync::queue_submit_callback (nullptr,
                                           composite,
                                           port::work,
                                           nullptr,
                                           &count) == S_OK);

    assert (xasync::queue_dispatch (nullptr, composite, port::work, 0));
    assert (ran.load (std::memory_order_relaxed) == 1);

    assert (xasync::queue_close_handle (nullptr, composite) == S_OK);
  }

}
