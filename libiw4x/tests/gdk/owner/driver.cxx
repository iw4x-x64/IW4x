#undef NDEBUG
#include <cassert>

#include <libiw4x/gdk/owner.hxx>

using namespace iw4x::gdk;

namespace
{
  int live (0);

  struct base
  {
    virtual
    ~base ()
    {
      --live;
    }

    base ()
    {
      ++live;
    }

    virtual int
    tag () const
    {
      return 1;
    }
  };

  struct derived: base
  {
    explicit
    derived (int v): value (v) {}

    int
    tag () const override
    {
      return value;
    }

    int value;
  };
}

int
main ()
{
  {
    owner<base> o;

    assert (!o);
    assert (o.get () == nullptr);
  }

  {
    owner<base> o (make<base> ());

    assert (o);
    assert (live == 1);
    assert (o->tag () == 1);
    assert ((*o).tag () == 1);
    assert (o.get () != nullptr);
  }

  assert (live == 0);

  {
    owner<base> o (make<base> ());

    o.reset ();

    assert (!o && live == 0);

    o.reset ();
    assert (!o);
  }

  {
    owner<base> o (make<base> ());

    base* p (o.release ());

    assert (!o && live == 1 && p != nullptr);

    delete p;
    assert (live == 0);
  }

  {
    owner<base> a (make<base> ());
    owner<base> b (static_cast<owner<base>&&> (a));

    assert (!a && b && live == 1);

    owner<base> c;
    c = static_cast<owner<base>&&> (b);

    assert (!b && c && live == 1);

    c = static_cast<owner<base>&&> (c);
    assert (c && live == 1);
  }

  assert (live == 0);

  {
    owner<derived> d (make<derived> (7));

    assert (d->value == 7);

    owner<base> b (static_cast<owner<derived>&&> (d));

    assert (!d && b);
    assert (b->tag () == 7);
    assert (live == 1);
  }

  assert (live == 0);

  {
    owner<derived> d (make<derived> (9));
    owner<base>    b;

    b = static_cast<owner<derived>&&> (d);

    assert (b->tag () == 9);
  }

  assert (live == 0);

  {
    owner<base> a (make<derived> (1));
    owner<base> b (make<derived> (2));

    assert (live == 2);

    a = static_cast<owner<base>&&> (b);

    assert (a->tag () == 2);
    assert (live == 1);
  }

  assert (live == 0);
}
