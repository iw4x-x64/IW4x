#undef NDEBUG
#include <cassert>

#include <cwchar>
#include <cstring>

#include <libiw4x/gdk/path.hxx>

using namespace iw4x::gdk;

int
main ()
{
  {
    path p;

    assert (p.empty ());
    assert (p.size () == 0);
    assert (p.whole ());
    assert (std::wcscmp (p.c_str (), L"") == 0);
  }

  {
    path p (L"C:\\one\\two");

    assert (!p.empty ());
    assert (std::wcscmp (p.c_str (), L"C:\\one\\two") == 0);
    assert (p.size () == 10);
  }

  {
    path p (L"C:\\one");

    p.append (L"two");
    assert (std::wcscmp (p.c_str (), L"C:\\one\\two") == 0);

    p.append (chars ("three"));
    assert (std::wcscmp (p.c_str (), L"C:\\one\\two\\three") == 0);
  }

  {
    path p (L"C:\\one\\");

    p.append (L"two");
    assert (std::wcscmp (p.c_str (), L"C:\\one\\two") == 0);
  }

  {
    path p (L"C:\\one\\two");

    p.extend (L".part");
    assert (std::wcscmp (p.c_str (), L"C:\\one\\two.part") == 0);
  }

  {
    path p (L"C:\\one\\two\\three");

    p.to_directory ();
    assert (std::wcscmp (p.c_str (), L"C:\\one\\two") == 0);

    p.to_directory ();
    assert (std::wcscmp (p.c_str (), L"C:\\one") == 0);

    p.to_directory ();
    assert (std::wcscmp (p.c_str (), L"C:") == 0);
  }

  {
    path p (L"C:\\one");

    p.clear ();

    assert (p.empty () && p.whole ());
  }

  {
    path p;

    for (int i (0); i != 200; ++i)
      p.append (L"component");

    assert (!p.whole ());
    assert (p.size () < path::capacity);
    assert (p.c_str ()[p.size ()] == L'\0');
  }

  {
    path p (L"C:\\caf\u00e9");

    char b[64];

    assert (chars (p.narrow (b, sizeof (b))) == chars ("C:\\caf\xC3\xA9"));
  }

  {
    path p;

    p.extend (chars ("caf\xC3\xA9"));

    assert (std::wcscmp (p.c_str (), L"caf\u00e9") == 0);
  }

  {
    char b[64];

    assert (narrow (L"one", b, sizeof (b)) == chars ("one"));
    assert (std::strcmp (b, "one") == 0);
  }
}
