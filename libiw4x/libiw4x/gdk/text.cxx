#include <libiw4x/gdk/text.hxx>

namespace iw4x
{
  namespace gdk
  {
    static constexpr std::size_t digits (21);

    void text_writer::
    write (chars v) noexcept
    {
      std::size_t n (v.size ());

      if (n > c_ - n_ - 1)
      {
        n = c_ - n_ - 1;
        truncated_ = true;
      }

      __builtin_memcpy (b_ + n_, v.data (), n);

      n_ += n;
      terminate ();
    }

    void text_writer::
    write (const char* v) noexcept
    {
      write (chars (v != nullptr ? v : "(null)"));
    }

    void text_writer::
    write (char v) noexcept
    {
      write (chars (&v, 1));
    }

    void text_writer::
    write (bool v) noexcept
    {
      write (chars (v ? "true" : "false"));
    }

    void text_writer::
    write (const void* v) noexcept
    {
      write (hex (reinterpret_cast<std::uintptr_t> (v)));
    }

    void text_writer::
    write (hex v) noexcept
    {
      static const char table[] = "0123456789ABCDEF";

      char  s[16];
      char* p (s + sizeof (s));

      std::uint64_t r (v.value);
      unsigned      w (v.width);

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

      write (chars ("0x"));
      write (chars (p, static_cast<std::size_t> (s + sizeof (s) - p)));
    }

    void text_writer::
    write_unsigned (std::uint64_t v) noexcept
    {
      char  s[digits];
      char* p (s + sizeof (s));

      do
      {
        *--p = static_cast<char> ('0' + (v % 10));
        v /= 10;
      }
      while (v != 0);

      write (chars (p, static_cast<std::size_t> (s + sizeof (s) - p)));
    }

    void text_writer::
    write_signed (std::int64_t v) noexcept
    {
      if (v < 0)
      {
        write (chars ("-"));
        write_unsigned (~static_cast<std::uint64_t> (v) + 1);
        return;
      }

      write_unsigned (static_cast<std::uint64_t> (v));
    }

    const char*
    write_literal (text_writer& w, const char* f) noexcept
    {
      const char* p (f);

      for (; p[0] != '\0'; ++p)
      {
        if (p[0] == '{' && p[1] == '}')
        {
          w.write (chars (f, static_cast<std::size_t> (p - f)));
          return p + 2;
        }
      }

      w.write (chars (f, static_cast<std::size_t> (p - f)));
      return p;
    }

    void
    write_rest (text_writer& w, const char* f) noexcept
    {
      w.write (chars (f));
    }
  }
}
