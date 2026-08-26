#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <libiw4x/gdk/path.hxx>

#undef NDEBUG
#include <cassert>

using namespace iw4x::gdk;

namespace
{
  path
  wide (const char* p)
  {
    path r;

    r.extend (chars (p));

    return r;
  }

  int
  fail (const char* what)
  {
    std::fprintf (stderr, "%s\n", what);

    return 1;
  }

  int
  make (const char* p)
  {
    return create_directories (wide (p)) ? 0 : fail ("unable to create");
  }

  int
  type (const char* p)
  {
    std::printf ("%s\n", is_directory (wide (p)) ? "dir" : "other");

    return 0;
  }

  int
  write (const char* p)
  {
    char        b[4096];
    std::size_t n (std::fread (b, 1, sizeof (b), stdin));

    file f;

    if (!f.open_write (wide (p)))
      return fail ("unable to open for writing");

    return f.write (b, n) ? 0 : fail ("unable to write");
  }

  int
  read (const char* p)
  {
    file f;

    if (!f.open_read (wide (p)))
      return fail ("unable to open for reading");

    std::uint64_t n (0);

    if (!f.size (n))
      return fail ("unable to size");

    blob b;

    if (!b.resize (static_cast<std::size_t> (n)))
      return fail ("unable to allocate");

    if (!b.empty () && !f.read (b.data (), b.size ()))
      return fail ("unable to read");

    std::fwrite (b.data (), 1, b.size (), stdout);

    return 0;
  }

  int
  size (const char* p)
  {
    file f;

    if (!f.open_read (wide (p)))
      return fail ("unable to open for reading");

    std::uint64_t n (0);

    if (!f.size (n))
      return fail ("unable to size");

    std::printf ("%llu\n", static_cast<unsigned long long> (n));

    return 0;
  }

  int
  list (const char* p)
  {
    directory d;

    if (!d.open (wide (p)))
      return fail ("unable to open directory");

    while (d.next ())
    {
      char b[512];

      std::printf ("%s %s %llu\n",
                   d.is_directory () ? "dir" : "reg",
                   narrow (d.name (), b, sizeof (b)).data (),
                   static_cast<unsigned long long> (
                     d.is_directory () ? 0 : d.size ()));
    }

    return 0;
  }

  int
  age (const char* p)
  {
    file f;

    if (!f.open_read (wide (p)))
      return fail ("unable to open for reading");

    std::printf ("%s\n",
                 f.last_write_seconds () > 1600000000 ? "recent" : "stale");

    return 0;
  }
}

int
main (int argc, const char* argv[])
{
  assert (argc == 3);

  const char* c (argv[1]);
  const char* p (argv[2]);

  if (std::strcmp (c, "mkdir") == 0) return make (p);
  if (std::strcmp (c, "type")  == 0) return type (p);
  if (std::strcmp (c, "write") == 0) return write (p);
  if (std::strcmp (c, "read")  == 0) return read (p);
  if (std::strcmp (c, "size")  == 0) return size (p);
  if (std::strcmp (c, "list")  == 0) return list (p);
  if (std::strcmp (c, "age")   == 0) return age (p);

  return fail ("unknown command");
}
