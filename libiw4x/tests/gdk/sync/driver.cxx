#undef NDEBUG
#include <cassert>

#include <atomic>

#include <libiw4x/gdk/sync.hxx>

using namespace iw4x::gdk;

namespace
{
  struct shared
  {
    mutex     mutex_;
    condition ready;

    bool      set = false;
    int       counter = 0;
  };

  shared state;

  std::atomic<int> started {0};

  void
  bump (void*) noexcept
  {
    started.fetch_add (1, std::memory_order_relaxed);

    for (int i (0); i != 1000; ++i)
    {
      scope_lock l (state.mutex_);
      ++state.counter;
    }
  }

  void
  waker (void*) noexcept
  {
    {
      scope_lock l (state.mutex_);
      state.set = true;
    }

    state.ready.wake_all ();
  }
}

int
main ()
{
  {
    mutex m;

    m.acquire ();
    m.release ();

    assert (m.native () != nullptr);
  }

  {
    mutex m;

    {
      scope_lock l (m);

      assert (&l.owner () == &m);
    }

    {
      scope_lock l (m);
      l.release ();
      l.release ();
    }

    scope_lock l (m);
  }

  {
    thread t;

    assert (!t.started ());

    t.join ();

    assert (t.start (&bump, nullptr));
    assert (t.started ());

    t.join ();
    t.join ();

    assert (!t.started ());
    assert (started.load () == 1);
    assert (state.counter == 1000);
  }

  {
    thread a, b, c;

    state.counter = 0;

    assert (a.start (&bump, nullptr));
    assert (b.start (&bump, nullptr));
    assert (c.start (&bump, nullptr));

    a.join ();
    b.join ();
    c.join ();

    assert (state.counter == 3000);
  }

  {
    thread t;

    assert (t.start (&waker, nullptr));

    {
      scope_lock l (state.mutex_);

      while (!state.set)
        state.ready.wait (l, INFINITE);
    }

    t.join ();

    assert (state.set);
  }

  {
    mutex     m;
    condition c;

    scope_lock l (m);

    std::uint64_t b (now ());

    assert (!c.wait (l, 50));

    assert (now () >= b);
  }

  {
    std::uint64_t a (now ());
    std::uint64_t b (now ());

    assert (b >= a);
  }
}
