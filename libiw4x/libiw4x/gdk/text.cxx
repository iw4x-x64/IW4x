#include <libiw4x/gdk/text.hxx>

#include <charconv>
#include <algorithm>

#include <libiw4x/hexadecimal.hxx>

using namespace std;

namespace iw4x
{
  namespace gdk
  {
    static constexpr size_t digits (21);

    void text_writer::
    write (string_view v) noexcept
    {
      LIBIW4X_INVARIANT (n_ < c_ && b_[n_] == '\0');

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
      write (hex_number (reinterpret_cast<uintptr_t> (v)));
    }

    void text_writer::
    write (hex_number v) noexcept
    {
      char s[16];

      const hex_to_chars_result r (
        to_hex_chars (s, s + sizeof (s), v.value, hex_case::upper));

      LIBIW4X_ASSERT (r.ec == errc ());

      size_t z (0);

      while (z != sizeof (s) - 1 && s[z] == '0')
        ++z;

      size_t n (max (sizeof (s) - z, min<size_t> (v.width, sizeof (s))));

      write (string_view ("0x"));
      write (string_view (s + sizeof (s) - n, n));
    }

    void text_writer::
    write_unsigned (uint64_t v) noexcept
    {
      char s[digits];

      to_chars_result r (to_chars (s, s + sizeof (s), v));

      LIBIW4X_ASSERT (r.ec == errc ());

      write (string_view (s, r.ptr));
    }

    void text_writer::
    write_signed (int64_t v) noexcept
    {
      char s[digits];

      to_chars_result r (to_chars (s, s + sizeof (s), v));

      LIBIW4X_ASSERT (r.ec == errc ());

      write (string_view (s, r.ptr));
    }

    const char*
    write_literal (text_writer& w, const char* f) noexcept
    {
      LIBIW4X_PRE (f != nullptr);

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
      LIBIW4X_PRE (f != nullptr);

      w.write (string_view (f));
    }
  }
}
