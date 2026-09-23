#include <libiw4x/gdk/text.hxx>

using namespace std;

namespace iw4x
{
  namespace gdk
  {
    static constexpr size_t digits (21);

    void text_writer::
    write (string_view v) noexcept
    {
      size_t n (v.size ());

      if (n > c_ - n_ - 1)
      {
        n = c_ - n_ - 1;
        truncated_ = true;
      }

      v.copy (b_ + n_, n);

      n_ += n;
      terminate ();
    }

    void text_writer::
    write (const char* v) noexcept
    {
      write (string_view (v != nullptr ? v : "(null)"));
    }

    void text_writer::
    write (char v) noexcept
    {
      write (string_view (&v, 1));
    }

    void text_writer::
    write (bool v) noexcept
    {
      write (string_view (v ? "true" : "false"));
    }

    void text_writer::
    write (const void* v) noexcept
    {
      write (hex (reinterpret_cast<uintptr_t> (v)));
    }

    void text_writer::
    write (hex v) noexcept
    {
      static const char table[] = "0123456789ABCDEF";

      char  s[16];
      char* p (s + sizeof (s));

      uint64_t r (v.value);
      unsigned w (v.width);

      do
      {
        *--p = table[r & 0xF];
        r >>= 4;

        if (w != 0)
          --w;
      }
      while (r != 0);

      for (; w != 0 && p != s; --w)
        *--p = '0';

      write (string_view ("0x"));
      write (string_view (p, static_cast<size_t> (s + sizeof (s) - p)));
    }

    void text_writer::
    write_unsigned (uint64_t v) noexcept
    {
      char  s[digits];
      char* p (s + sizeof (s));

      do
      {
        *--p = static_cast<char> ('0' + (v % 10));
        v /= 10;
      }
      while (v != 0);

      write (string_view (p, static_cast<size_t> (s + sizeof (s) - p)));
    }

    void text_writer::
    write_signed (int64_t v) noexcept
    {
      if (v < 0)
      {
        write (string_view ("-"));
        write_unsigned (~static_cast<uint64_t> (v) + 1);
        return;
      }

      write_unsigned (static_cast<uint64_t> (v));
    }

    const char*
    write_literal (text_writer& w, const char* f) noexcept
    {
      const char* p (f);

      for (; p[0] != '\0'; ++p)
      {
        if (p[0] == '{' && p[1] == '}')
        {
          w.write (string_view (f, static_cast<size_t> (p - f)));
          return p + 2;
        }
      }

      w.write (string_view (f, static_cast<size_t> (p - f)));
      return p;
    }

    void
    write_rest (text_writer& w, const char* f) noexcept
    {
      w.write (string_view (f));
    }
  }
}
