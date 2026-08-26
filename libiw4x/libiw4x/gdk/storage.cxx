#include <libiw4x/gdk/storage.hxx>

#include <libiw4x/logger.hxx>

#include <libiw4x/gdk/error.hxx>
#include <libiw4x/gdk/types.hxx>

namespace iw4x
{
  namespace gdk
  {
    namespace
    {
      path
      resolve_root ()
      {
        wchar_t m[path::capacity];

        DWORD n (GetModuleFileNameW (image (),
                                     m,
                                     static_cast<DWORD> (path::capacity)));

        if (n == 0 || n == path::capacity)
          raise_win32 ("unable to retrieve module location");

        path r;

        r.extend (m, n);
        r.to_directory ();
        r.append (L"players");

        if (!r.whole ())
          raise (E_FAIL, "the storage root does not fit in a path");

        if (!create_directories (r))
          raise_win32 ("unable to create the storage root");

        char b[path::capacity];

        info ("storage root is {}", r.narrow (b, sizeof (b)).data ());

        return r;
      }
    }

    const path&
    storage_root ()
    {
      static const path r (resolve_root ());
      return r;
    }
  }
}
