#include <libiw4x/hexadecimal.hxx>

#include <algorithm> // ranges::copy(), min()
#include <ostream>
#include <sstream>
#include <stdexcept> // invalid_argument
#include <utility>   // to_underlying()

using namespace std;

namespace iw4x
{
  string
  hex_encode (span<const byte> b, const hex_style& s)
  {
    LIBIW4X_PRE (s.valid ());

    const size_t n (hex_encoded_size (b.size (), s));

    string r (n, '\0');

    const hex_to_chars_result t (
      to_hex_chars (r.data (), r.data () + r.size (), b.data (), b.size (), s));

    LIBIW4X_ASSERT (t.ec == errc ());
    LIBIW4X_ASSERT (t.ptr == r.data () + n);

    return r;
  }

  static optional<vector<byte>>
  decode (string_view s, const hex_style& st)
  {
    LIBIW4X_PRE (st.valid ());

    size_t n (s.size ());

    if (st.prefix)
    {
      if (n < 2)
        return nullopt;

      n -= 2;
    }

    size_t c;

    if (st.separator == '\0')
      c = n / 2;
    else
    {
      const size_t g (st.group);
      const size_t unit (2 * g + 1);

      c = ((n + 1) / unit) * g;

      const size_t rem ((n + 1) % unit);

      if (rem != 0)
      {
        if (rem % 2 == 0)
          return nullopt;

        c += (rem - 1) / 2;
      }
    }

    if (hex_encoded_size (c, st) != s.size ())
      return nullopt;

    vector<byte> r (c);

    const hex_from_chars_result t (
      from_hex_chars (s.data (), s.data () + s.size (), r.data (), c, st));

    if (t.ec != errc ())
      return nullopt;

    LIBIW4X_ASSERT (t.ptr == s.data () + s.size ());

    return r;
  }

  vector<byte>
  hex_decode (string_view s, const hex_style& st)
  {
    if (optional<vector<byte>> r = decode (s, st))
      return move (*r);

    throw invalid_argument ("invalid hexadecimal representation");
  }

  optional<vector<byte>>
  try_hex_decode (string_view s, const hex_style& st)
  {
    return decode (s, st);
  }

  optional<vector<byte>>
  try_hex_decode_relaxed (string_view s, string_view ignored)
  {
    size_t d (0);
    const char* p (s.data ());
    const char* e (p + s.size ());

    if (e - p >= 2 && p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
      p += 2;

    for (const char* i (p); i != e; ++i)
    {
      if (hex_digit_p (*i))
        ++d;
      else if (ignored.find (*i) == string_view::npos)
        return nullopt;
    }

    if (d % 2 != 0)
      return nullopt;

    vector<byte> r (d / 2);

    const hex_from_chars_result t (
      from_hex_chars_relaxed (p, e, r.data (), r.size (), ignored));

    if (t.ec != errc ())
      return nullopt;

    for (const char* i (t.ptr); i != e; ++i)
      LIBIW4X_ASSERT (ignored.find (*i) != string_view::npos);

    return r;
  }

  ostream&
  operator<< (ostream& os, const hex_view& v)
  {
    const string s (hex_encode (v.octets, v.style));

    return os.write (s.data (), static_cast<streamsize> (s.size ()));
  }

  string hex_dumper::
  dump (span<const byte> b, const hex_dump_options& o) const
  {
    ostringstream os;
    dump (os, b, o);
    return move (os).str ();
  }

  namespace
  {
    constexpr char
    printable (byte b) noexcept
    {
      const unsigned char c (static_cast<unsigned char> (b));

      return (c >= 0x20 && c < 0x7f) ? static_cast<char> (c) : '.';
    }

    size_t
    offset_width (size_t last) noexcept
    {
      size_t w (1);

      while (last >= 16)
      {
        last >>= 4;
        ++w;
      }

      return w <= 8 ? 8 : ((w + 3) / 4) * 4;
    }

    void
    annotate (ostream& os, const hex_dump_options& o, size_t off)
    {
      if (o.annotate)
      {
        const string n (o.annotate (off));

        if (!n.empty ())
          os << "  ; " << n;
      }
    }
  }

  void canonical_hex_dumper::
  dump (ostream& os, span<const byte> b, const hex_dump_options& o) const
  {
    LIBIW4X_PRE (o.valid ());

    const size_t cols (o.columns);
    const size_t w (offset_width (o.offset + b.size ()));

    string line;
    line.reserve (cols * 3 + cols / 8 + 1);

    for (size_t i (0); i < b.size (); i += cols)
    {
      const size_t n (min (cols, b.size () - i));
      const size_t off (o.offset + i);

      line.clear ();

      for (size_t j (0), e (o.ascii ? cols : n); j != e; ++j)
      {
        if (j != 0)
        {
          line += ' ';

          if (o.group != 0 && j % o.group == 0)
            line += ' ';
        }

        if (j < n)
        {
          const uint8_t v (hex_octet (b[i + j]));

          line += hex_digit (v >> 4, o.digit_case);
          line += hex_digit (v & 0x0f, o.digit_case);
        }
        else
        {
          line += "  ";
        }
      }

      if (o.offsets)
      {
        char ob[sizeof (size_t) * 2];

        const hex_to_chars_result t (
          to_hex_chars (ob, ob + sizeof (ob), off, o.digit_case));

        LIBIW4X_ASSERT (t.ec == errc ());
        LIBIW4X_ASSERT (w <= sizeof (ob));

        os.write (ob + (sizeof (ob) - w), static_cast<streamsize> (w));
        os << "  ";
      }

      os.write (line.data (), static_cast<streamsize> (line.size ()));

      if (o.ascii)
      {
        os << "  |";

        for (size_t j (0); j != n; ++j)
          os << printable (b[i + j]);

        os << '|';
      }

      annotate (os, o, off);

      os << '\n';
    }
  }

  void plain_hex_dumper::
  dump (ostream& os, span<const byte> b, const hex_dump_options& o) const
  {
    LIBIW4X_PRE (o.valid ());

    const size_t cols (o.columns);

    for (size_t i (0); i < b.size (); i += cols)
    {
      const size_t n (min (cols, b.size () - i));

      const string s (hex_encode (b.subspan (i, n),
                                  hex_style {o.digit_case, '\0', 1, false}));

      os.write (s.data (), static_cast<streamsize> (s.size ()));

      annotate (os, o, o.offset + i);

      os << '\n';
    }
  }

  void c_array_hex_dumper::
  dump (ostream& os, span<const byte> b, const hex_dump_options& o) const
  {
    LIBIW4X_PRE (o.valid ());

    const size_t cols (o.columns);

    for (size_t i (0); i < b.size (); i += cols)
    {
      const size_t n (min (cols, b.size () - i));

      os << ' ' << ' ';

      for (size_t j (0); j != n; ++j)
      {
        const uint8_t v (hex_octet (b[i + j]));

        if (j != 0)
          os << ' ';

        os << "0x"
           << hex_digit (v >> 4, o.digit_case)
           << hex_digit (v & 0x0f, o.digit_case);

        os << ',';
      }

      annotate (os, o, o.offset + i);

      os << '\n';
    }
  }

  const hex_dumper&
  hex_dumper_for (hex_dump_style s) noexcept
  {
    static const canonical_hex_dumper canonical;
    static const plain_hex_dumper     plain;
    static const c_array_hex_dumper   c_array;

    switch (s)
    {
      case hex_dump_style::canonical: return canonical;
      case hex_dump_style::plain:     return plain;
      case hex_dump_style::c_array:   return c_array;
    }

    LIBIW4X_UNREACHABLE ();
  }

  unique_ptr<hex_dumper>
  make_hex_dumper (hex_dump_style s)
  {
    switch (s)
    {
      case hex_dump_style::canonical:
        return make_unique<canonical_hex_dumper> ();

      case hex_dump_style::plain:
        return make_unique<plain_hex_dumper> ();

      case hex_dump_style::c_array:
        return make_unique<c_array_hex_dumper> ();
    }

    LIBIW4X_UNREACHABLE ();
  }

  string
  hex_dump (span<const byte> b, const hex_dump_options& o)
  {
    return hex_dumper_for (hex_dump_style::canonical).dump (b, o);
  }
}
