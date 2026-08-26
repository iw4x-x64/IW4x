#undef NDEBUG
#include <cassert>

#include <cstring>

#include <libiw4x/gdk/error.hxx>
#include <libiw4x/gdk/types.hxx>
#include <libiw4x/gdk/argument.hxx>

using namespace iw4x::gdk;

int
main ()
{
  {
    failure f (E_ACCESSDENIED, "cannot open '{}'", "name");

    assert (f.code () == E_ACCESSDENIED);
    assert (std::strcmp (f.what (), "cannot open 'name'") == 0);

    f.append (": {}", 42);

    assert (std::strcmp (f.what (), "cannot open 'name': 42") == 0);
  }

  {
    bool caught (false);

    try
    {
      raise (E_NOTIMPL, "nothing to do");
    }
    catch (const failure& e)
    {
      caught = true;

      assert (e.code () == E_NOTIMPL);
      assert (std::strcmp (e.what (), "nothing to do") == 0);
    }

    assert (caught);
  }

  {
    bool caught (false);

    try
    {
      raise_invalid ("{} is not a slot", 3);
    }
    catch (const failure& e)
    {
      caught = true;

      assert (e.code () == E_INVALIDARG);
      assert (std::strcmp (e.what (), "3 is not a slot") == 0);
    }

    assert (caught);
  }

  {
    SetLastError (ERROR_FILE_NOT_FOUND);

    bool caught (false);

    try
    {
      raise_win32 ("unable to open");
    }
    catch (const failure& e)
    {
      caught = true;

      assert (e.code () == HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND));
      assert (std::strncmp (e.what (), "unable to open: ", 16) == 0);
      assert (std::strlen (e.what ()) > 16);
    }

    assert (caught);
  }

  {
    HRESULT r (guard ("body", [] () -> HRESULT { return S_FALSE; }));
    assert (r == S_FALSE);

    r = guard ("raising", [] () -> HRESULT
    {
      raise (E_OUTOFMEMORY, "no room");
    });

    assert (r == E_OUTOFMEMORY);

    r = guard ("throwing", [] () -> HRESULT
    {
      throw 7;
    });

    assert (r == E_UNEXPECTED);
  }

  {
    int ran (0);

    guard ("void body", [&ran] () -> void { ++ran; });
    assert (ran == 1);

    guard ("void raising", [&ran] () -> void
    {
      ++ran;
      raise (E_FAIL, "no");
    });

    assert (ran == 2);
  }

  {
    char r (guard ("valued", char (9), [] () -> char { return 1; }));
    assert (r == 1);

    r = guard ("valued raising", char (9), [] () -> char
    {
      raise (E_FAIL, "no");
    });

    assert (r == 9);

    bool b (guard ("valued bool", false, [] () -> bool
    {
      throw 0;
    }));

    assert (!b);
  }

  {
    int v (7);

    assert (&answer (&v) == &v);
    assert (answer (&v) == 7);

    bool caught (false);

    try
    {
      answer (static_cast<int*> (nullptr));
    }
    catch (const failure& e)
    {
      caught = true;
      assert (e.code () == E_POINTER);
    }

    assert (caught);
  }

  {
    char        b[16];
    std::size_t used (0);

    copy_out (chars ("value"), sizeof (b), b, &used);

    assert (std::strcmp (b, "value") == 0);
    assert (used == 6);

    copy_out (chars ("value"), sizeof (b), b, nullptr);
    assert (std::strcmp (b, "value") == 0);

    copy_out (chars (""), sizeof (b), b, &used);
    assert (b[0] == '\0' && used == 1);
  }

  {
    char b[4];

    bool caught (false);

    try
    {
      copy_out (chars ("value"), sizeof (b), b, nullptr);
    }
    catch (const failure& e)
    {
      caught = true;
      assert (e.code () == insufficient_buffer);
    }

    assert (caught);

    caught = false;

    try
    {
      copy_out (chars ("value"), 16, nullptr, nullptr);
    }
    catch (const failure& e)
    {
      caught = true;
      assert (e.code () == E_POINTER);
    }

    assert (caught);
  }

  {
    guid a {0x11223344, 0x5566, 0x7788,
            {0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00}};

    guid b (a);

    assert (a == b);

    b.data4[7] = 1;
    assert (!(a == b));

    b = a;
    b.data1 = 0;
    assert (!(a == b));

    static_assert (guid {1, 2, 3, {4, 5, 6, 7, 8, 9, 10, 11}} ==
                   guid {1, 2, 3, {4, 5, 6, 7, 8, 9, 10, 11}});

    static_assert (!(guid {1, 2, 3, {4, 5, 6, 7, 8, 9, 10, 11}} ==
                     guid {1, 2, 3, {4, 5, 6, 7, 8, 9, 10, 12}}));
  }

  {
    assert (std::strcmp (name (feature::async), "XAsync") == 0);
    assert (std::strcmp (name (feature::game_streaming), "XGameStreaming") ==
            0);
    assert (std::strcmp (name (static_cast<feature> (99)),
                         "<unrecovered family>") == 0);
  }

  {
    assert (pending == static_cast<HRESULT> (0x8000000AL));
    assert (image () != nullptr);
  }
}
