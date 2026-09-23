#undef NDEBUG
#include <cassert>

#include <cstring>

#include <libiw4x/gdk/path.hxx>
#include <libiw4x/gdk/storage.hxx>
#include <libiw4x/gdk/identity.hxx>

using namespace iw4x::gdk;

int
main ()
{
  {
    const path& r (storage_root ());

    assert (!r.empty ());
    assert (is_directory (r));

    assert (&storage_root () == &r);
  }

  {
    path p (storage_root ());
    p.append (L"identity");

    file f;

    assert (f.open_write (p));
    assert (f.write ("0x0009000012345678\r\n", 20));
  }

  {
    std::uint64_t x (xuid ());

    assert (x == 0x0009000012345678ULL);
    assert ((x >> 48) == 9);
    assert (xuid () == x);
  }

  {
    char b[gamertag_capacity];

    std::size_t n (gamertag (b, sizeof (b)));

    assert (n != 0 && std::strlen (b) == n);
    assert (std::strncmp (b, "IW4x-", 5) == 0);

    set_gamertag ("Someone");

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
    set_gamertag ("\xC3\xA9\xC3\xA9\xC3\xA9");

    char b[gamertag_capacity];

    std::size_t n (gamertag (b, 4));

    assert (n == 2);
    assert (std::strcmp (b, "\xC3\xA9") == 0);
  }
}
