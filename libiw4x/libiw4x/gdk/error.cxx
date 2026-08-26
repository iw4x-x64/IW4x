#include <libiw4x/gdk/error.hxx>

#include <cstdio>
#include <string_view>

#include <quill/Backend.h>

#include <libiw4x/logger.hxx>

namespace iw4x
{
  namespace gdk
  {
    void
    append_last_error (failure& r, DWORD e) noexcept
    {
      char* b (nullptr);

      DWORD n (FormatMessageA (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                               FORMAT_MESSAGE_FROM_SYSTEM     |
                               FORMAT_MESSAGE_IGNORE_INSERTS,
                               nullptr,
                               e,
                               0,
                               reinterpret_cast<char*> (&b),
                               0,
                               nullptr));

      if (n == 0 || b == nullptr)
      {
        r.append (": error {}", e);
        return;
      }

      while (n != 0 && (b[n - 1] == '\n' ||
                        b[n - 1] == '\r' ||
                        b[n - 1] == '.'))
        --n;

      r.append (": {}", chars (b, n));

      LocalFree (b);
    }

    void
    fatal (chars what) noexcept
    {
      fail ("{}", std::string_view (what.data (), what.size ()));

      quill::Backend::stop ();

      text<description_size> m ("{}", what);

      std::fprintf (stderr, "iw4x: %s\n", m.c_str ());
      std::fflush (stderr);

      if (GetConsoleWindow () == nullptr)
        MessageBoxA (nullptr,
                     m.c_str (),
                     "IW4x",
                     MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);

      __fastfail (FAST_FAIL_FATAL_APP_EXIT);
    }

    HRESULT
    report (const char* p) noexcept
    {
      try
      {
        throw;
      }
      catch (const failure& e)
      {
        l1 ("{}: {} ({:#010x})",
            p,
            e.what (),
            static_cast<std::uint32_t> (e.code ()));

        return e.code ();
      }
      catch (...)
      {
        fail ("{}: unknown error", p);

        return E_UNEXPECTED;
      }
    }
  }
}
