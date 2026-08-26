#undef NDEBUG
#include <cassert>

#include <libiw4x/gdk/handler.hxx>

using namespace iw4x::gdk;

namespace
{
  struct entry
  {
    void* context;
    int   tag;
  };
}

int
main ()
{
  {
    handler_table<entry, 4> t;

    assert (t.size () == 0);

    std::uint64_t a (t.add (entry {nullptr, 1}));
    std::uint64_t b (t.add (entry {nullptr, 2}));

    assert (a == 1 && b == 2);
    assert (t.size () == 2);

    entry       live[4];
    std::size_t n (t.live (live, 4));

    assert (n == 2);
    assert (live[0].tag == 1 && live[1].tag == 2);
  }

  {
    handler_table<entry, 2> t;

    assert (t.add (entry {nullptr, 1}) == 1);
    assert (t.add (entry {nullptr, 2}) == 2);
    assert (t.add (entry {nullptr, 3}) == 0);

    assert (t.size () == 2);
  }

  {
    handler_table<entry, 4> t;

    std::uint64_t a (t.add (entry {nullptr, 1}));
    std::uint64_t b (t.add (entry {nullptr, 2}));

    assert (t.remove (a));
    assert (!t.remove (a));
    assert (t.size () == 1);

    assert (t.add (entry {nullptr, 3}) == a);
    assert (t.size () == 2);

    entry       live[4];
    std::size_t n (t.live (live, 4));

    assert (n == 2);
    assert (live[0].tag == 3 && live[1].tag == 2);

    assert (t.remove (b));
    assert (t.size () == 1);
  }

  {
    handler_table<entry, 4> t;

    assert (!t.remove (0));
    assert (!t.remove (1));
    assert (!t.remove (5));
    assert (!t.remove (~std::uint64_t (0)));
  }

  {
    handler_table<entry, 4> t;

    t.add (entry {nullptr, 1});
    t.add (entry {nullptr, 2});
    t.add (entry {nullptr, 3});

    entry       live[2];
    std::size_t n (t.live (live, 2));

    assert (n == 2);
  }

  {
    using small    = handler_table<entry, 4>;
    using standard = handler_table<entry>;

    static_assert (small::capacity == 4);
    static_assert (standard::capacity == 8);
  }
}
