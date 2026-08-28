#include <libiw4x/gdk/error.hxx>

#include <cstdio>
#include <string_view>

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

      logger::stop ();

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

    namespace
    {
      thread_local const char* failed_entry (nullptr);
      thread_local HRESULT     failed_code (S_OK);
    }

    void
    note_failure (const char* p, HRESULT c) noexcept
    {
      failed_entry = p;
      failed_code  = c;
    }

    const char*
    last_failure (HRESULT c) noexcept
    {
      return failed_entry != nullptr && failed_code == c ? failed_entry
                                                         : nullptr;
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
        HRESULT c (e.code ());

        // Two statuses arrive here on a path the caller chose.
        //
        // XSAPI learns how long a name is by calling with a buffer too
        // short to hold one and reading the answer. 0x1402F59C0 does
        // exactly that before it decides whether it got a name, so
        // ERROR_INSUFFICIENT_BUFFER is a step on the way to a
        // successful call.
        //
        // E_ABORT is the answer of a queue that is already going down,
        // and every shutdown reaches it.
        //
        // Note that every other status here belongs to a call that
        // could not be carried out.
        //
        if (c == insufficient_buffer || c == E_ABORT)
          l1 ("{}: {} ({:#010x})",
              p,
              e.what (),
              static_cast<std::uint32_t> (c));
        else
          fail ("{}: {} ({:#010x})",
                p,
                e.what (),
                static_cast<std::uint32_t> (c));

        note_failure (p, c);
        return c;
      }
      catch (...)
      {
        fail ("{}: unknown error", p);

        note_failure (p, E_UNEXPECTED);
        return E_UNEXPECTED;
      }
    }
  }
}
