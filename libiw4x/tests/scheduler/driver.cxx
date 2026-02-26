#include <libiw4x/scheduler.hxx>

#undef NDEBUG
#include <cassert>

struct token
{
  int v;

  token (int v) : v (v) {}
  token (token&&) = default;
  token& operator= (token&&) = default;

  token (const token&) = delete;
  token& operator= (const token&) = delete;
};

struct tracker
{
  std::atomic<int>* c;

  tracker (std::atomic<int>& c) : c (&c) {}

  tracker (tracker&& o) noexcept
      : c (o.c)
  {
    o.c = nullptr;
  }

  tracker& operator= (tracker&& o) noexcept
  {
    if (this != &o)
    {
      if (c) (*c)++;
      c = o.c;
      o.c = nullptr;
    }
    return *this;
  }

  ~tracker ()
  {
    if (c) (*c)++;
  }

  tracker (const tracker&) = delete;
  tracker& operator= (const tracker&) = delete;
};

int
main ()
{
  using namespace std;
  using namespace iw4x;

  active_logger = new logger;

  // Test high contention.
  //
  {
    logical_scheduler s;
    atomic<int> n (0);
    const int nt (8);
    const int np (10000);

    vector<thread> ts;
    ts.reserve(nt);

    for (int i (0); i < nt; ++i)
    {
      ts.emplace_back ([&s, &n, np] ()
      {
        for (int j (0); j < np; ++j)
        {
          s.post ([&n] () { n++; }, asynchronous {});
        }
      });
    }

    for (auto& t : ts)
      t.join ();

    s.tick ();
    assert (n == nt * np);
  }

  // Test execution snapshotting.
  //
  {
    logical_scheduler s;
    bool l (false); // local.
    bool a (false); // async.

    s.post ([&] ()
    {
      s.post ([&l] () { l = true; });
      s.post ([&a] () { a = true; }, asynchronous {});
    });

    s.tick ();

    // Children should be pending.
    //
    assert (!l);
    assert (!a);

    s.tick ();

    assert (l);
    assert (a);
  }

  // Test move-only closures.
  //
  {
    logical_scheduler s;
    auto p (make_unique<token> (42));
    bool r (false);

    s.post ([p = move (p), &r] ()
    {
      assert (p->v == 42);
      r = true;
    });

    s.tick ();
    assert (r);
  }

  // Test destruction cleanup.
  //
  {
    atomic<int> n (0);
    {
      logical_scheduler s;
      s.post ([t = tracker (n)] () {});
      s.post ([t = tracker (n)] () {}, asynchronous {});
    }

    assert (n == 2);
  }

  // Test vector resizing.
  //
  {
    logical_scheduler s;
    int n (0);
    const int max (100000);

    for (int i (0); i < max; ++i)
    {
      s.post ([&n] () { n++; });
    }

    s.tick ();
    assert (n == max);
  }

  // Test exception propagation.
  //
  {
    logical_scheduler s;
    bool c (false);

    s.post ([] () { throw runtime_error ("boom"); });

    // This one shouldn't run this tick, but shouldn't leak either.
    //
    bool r (false);
    s.post ([&r] () { r = true; });

    try
    {
      s.tick ();
    }
    catch (const runtime_error&)
    {
      c = true;
    }

    assert (c);
  }

  // Test async FIFO ordering.
  //
  {
    logical_scheduler s;
    vector<int> v;

    s.post ([&] () { v.push_back (1); }, asynchronous {});
    s.post ([&] () { v.push_back (2); }, asynchronous {});
    s.post ([&] () { v.push_back (3); }, asynchronous {});

    s.tick ();

    assert (v.size () == 3);
    assert (v[0] == 1);
    assert (v[1] == 2);
    assert (v[2] == 3);
  }

    // Test repeating tasks.
  //
  {
    logical_scheduler s;
    int r (0);
    int o (0);

    s.post ([&r] () { r++; }, repeat_every_tick {});
    s.post ([&o] () { o++; });

    // First tick executes both the repeating and the one-shot task.
    //
    s.tick ();

    assert (r == 1);
    assert (o == 1);

    // Second tick executes only the repeating task.
    //
    s.tick ();

    assert (r == 2);
    assert (o == 1);

    // Third tick executes the repeating task again.
    //
    s.tick ();

    assert (r == 3);
    assert (o == 1);
  }
}
