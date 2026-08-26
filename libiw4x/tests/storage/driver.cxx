#undef NDEBUG
#include <cassert>

#include <cwchar>
#include <cstring>

#include <libiw4x/gdk/path.hxx>
#include <libiw4x/gdk/storage.hxx>
#include <libiw4x/gdk/identity.hxx>

using namespace iw4x::gdk;

int
main ()
{
  {
    path p (L"C:\\one\\two");

    assert (std::wcscmp (p.c_str (), L"C:\\one\\two") == 0);

    p.append (L"three");
    assert (std::wcscmp (p.c_str (), L"C:\\one\\two\\three") == 0);

    p.to_directory ();
    assert (std::wcscmp (p.c_str (), L"C:\\one\\two") == 0);

    p.append (chars ("four"));
    assert (std::wcscmp (p.c_str (), L"C:\\one\\two\\four") == 0);

    assert (p.whole ());

    char b[64];
    assert (chars (p.narrow (b, sizeof (b))) == chars ("C:\\one\\two\\four"));
  }

  {
    path p;

    assert (p.empty ());

    for (int i (0); i != 200; ++i)
      p.append (L"component");

    assert (!p.whole ());
    assert (p.size () < path::capacity);
  }

  {
    const path& r (storage_root ());

    assert (!r.empty ());
    assert (is_directory (r));

    assert (&storage_root () == &r);
  }

  {
    path d (storage_root ());
    d.append (L"test-scratch");

    assert (create_directories (d));
    assert (create_directories (d));
    assert (is_directory (d));

    path f (d);
    f.append (L"blob");

    {
      const char x[] = "the quick brown fox";

      file w;
      assert (w.open_write (f));
      assert (w.write (x, sizeof (x)));
    }

    {
      file q;
      assert (q.open_read (f));

      std::uint64_t s (0);
      assert (q.size (s) && s == 20);

      assert (q.last_write_seconds () > 1600000000);

      blob b;
      assert (b.resize (static_cast<std::size_t> (s)));
      assert (q.read (b.data (), b.size ()));

      assert (std::strcmp (reinterpret_cast<const char*> (b.data ()),
                           "the quick brown fox") == 0);
    }

    {
      directory e;
      assert (e.open (d));

      int found (0);

      while (e.next ())
      {
        if (std::wcscmp (e.name (), L"blob") == 0)
        {
          ++found;
          assert (!e.is_directory ());
          assert (e.size () == 20);
          assert (e.last_write_seconds () > 1600000000);
        }
      }

      assert (found == 1);
    }

    DeleteFileW (f.c_str ());
    RemoveDirectoryW (d.c_str ());
  }

  {
    blob b;

    assert (b.empty ());
    assert (b.resize (128));
    assert (b.size () == 128);

    b.data ()[0] = 7;
    assert (b.data ()[0] == 7);

    b.clear ();
    assert (b.empty () && b.data () == nullptr);
  }

  {
    std::uint64_t x (xuid ());

    assert (x != 0);
    assert ((x >> 48) == 9);
    assert (xuid () == x);
  }

  {
    char b[gamertag_capacity];

    std::size_t n (gamertag (b, sizeof (b)));

    assert (n != 0 && std::strlen (b) == n);
    assert (std::strncmp (b, "IW4x-", 5) == 0);

    set_gamertag (chars ("Someone"));

    n = gamertag (b, sizeof (b));
    assert (std::strcmp (b, "Someone") == 0 && n == 7);

    n = gamertag (b, 5);
    assert (std::strcmp (b, "Some") == 0 && n == 4);
  }

  {
    std::size_t n (0);

    assert (component_bound (gamertag_component::classic, n) && n == 15);
    assert (component_bound (gamertag_component::modern, n) && n == 96);
    assert (component_bound (gamertag_component::modern_suffix, n) && n == 0);
    assert (component_bound (gamertag_component::unique_modern, n) && n == 100);

    assert (!component_bound (static_cast<gamertag_component> (9), n));
  }

  {
    set_gamertag (chars ("\xC3\xA9\xC3\xA9\xC3\xA9"));

    char b[gamertag_capacity];

    std::size_t n (gamertag (b, 4));

    assert (n == 2);
    assert (std::strcmp (b, "\xC3\xA9") == 0);
  }
}
