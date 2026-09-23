#ifdef _WIN32
#  include <io.h>     // _setmode()
#  include <fcntl.h>  // _O_BINARY
#endif

#undef NDEBUG
#include <cassert>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>   // exit(), strtoull()
#include <cstring>   // memcmp(), memset()
#include <format>
#include <iostream>
#include <iterator>  // istreambuf_iterator
#include <limits>
#include <optional>
#include <random>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <libiw4x/hexadecimal.hxx>

using namespace std;
using namespace iw4x;

// Hexadecimal codec test driver.
//
// This driver is kept deliberately close to the public hexadecimal
// interface. Most tests feed bytes through the same entry points that a
// caller would use and compare the result with literal output in the
// testscript. This makes it fairly easy to tell whether a failure
// belongs to the codec or to the test driver itself.
//
// There are a few properties that don't fit that form very well. In
// particular, some implementation paths depend on the input position or
// on every possible byte value. Those are checked by verify() below
// where a loop can state the property directly.
//
// Usage: argv[0] <mode> [<option>...] [<argument>...]
//
// Modes:
//
//   encode   Read octets from stdin and write their hexadecimal
//            representation to stdout, followed by a newline.
//
//   decode   Read a hexadecimal representation from stdin (a single
//            trailing newline, if any, is ignored) and write the octets
//            to stdout with no trailing newline.
//
//   integer  Read numbers from stdin, one per line, and write each one
//            back as hexadecimal. Decimal and 0x-prefixed input are
//            both accepted.
//
//   size     Write the number of characters the octet count given as
//            the sole argument occupies in the selected style. Options
//            come before it.
//
//   dump     Read octets from stdin and write a hex dump to stdout.
//
//   format   Read octets from stdin and write one line per
//            std::format() specifier, each in the form "<spec>
//            <result>".
//
//   styles   Write the style table, one style per line, in the form
//            "<name> <specifier> <separator> <group> <prefix>".
//
//   verify   Run the internal consistency checks and write nothing. See
//            the comment on that function for the properties checked
//            there.
//
// Style options (encode, decode, size):
//
//   -s <style>  One of the names the styles mode lists. Default: compact.
//   -u          Upper-case digits.
//   -c <char>   Override the separator. '-c ""' removes it.
//   -g <n>      Override the group size.
//   -x          Prepend "0x".
//
// Decode options:
//
//   -r          Relaxed: ignore separators and an optional "0x" prefix.
//
// Integer options:
//
//   -b <bits>   Width: 8, 16, 32 or 64. Default: 64.
//   -m          Minimal width rather than fixed width.
//   -d          Decode: read hexadecimal and write decimal.
//
// Dump options:
//
//   -l <layout> canonical, plain or c_array. Default: canonical.
//   -w <n>      Octets per line.
//   -g <n>      Group size. Zero disables the group break.
//   -O <n>      Offset reported for the first octet.
//   -N          Omit the offset column.
//   -A          Omit the printable column.
//   -a          Annotate the first line.
//
// A malformed input or option is diagnosed on stderr and exits with 1.
//
// Everything below the allocating interface is usable during constant
// evaluation. Keep a small set of basic properties here so changes to
// those entry points are checked by the compiler before the runtime
// driver gets a chance to run.
//
// Note that a contract failure during constant evaluation makes the
// translation ill-formed. These assertions consequently check the
// result and establish that none of the expressions violates its
// contract.
//
static_assert (sizeof (hex_style) == 4);
static_assert (sizeof (hex_style_traits) == 16);

static_assert (hex_digit (0) == '0');
static_assert (hex_digit (10) == 'a');
static_assert (hex_digit (15, hex_case::upper) == 'F');
static_assert (hex_value ('0') == 0);
static_assert (hex_value ('f') == 15);
static_assert (hex_value ('F') == 15);
static_assert (hex_value ('g') == hex_invalid);
static_assert (hex_value ('\0') == hex_invalid);
static_assert (hex_digit_p ('9') && !hex_digit_p ('x'));
static_assert (hex_nibble ('a') == 10u);
static_assert (!hex_nibble ('z'));

static_assert (hex_encoded_size (4) == 8);
static_assert (hex_encoded_size (4, style (hex_style_id::colon)) == 11);
static_assert (hex_encoded_size (4, style (hex_style_id::prefixed)) == 10);
static_assert (hex_encoded_size (0, style (hex_style_id::colon)) == 0);
static_assert (hex_decoded_size (8) == 4);

static_assert (string_view (hex_case_name (hex_case::lower)) == "lower");
static_assert (string_view (hex_style_name (hex_style_id::colon)) == "colon");
static_assert (string_view (hex_dump_style_name (hex_dump_style::plain)) ==
               "plain");

// The compact round trip. Nine octets, so this also covers the tail the
// word-at-a-time path leaves behind.
//
static_assert ([] ()
{
  constexpr size_t n (9);

  const uint8_t b[n] {0xde, 0xad, 0xbe, 0xef, 0x00, 0x01, 0x7f, 0x80, 0xff};
  char c[n * 2] {};

  if (!to_hex_chars (c, c + sizeof (c), b, n))
    return false;

  if (string_view (c, sizeof (c)) != "deadbeef00017f80ff")
    return false;

  uint8_t o[n] {};

  if (!from_hex_chars (c, c + sizeof (c), o, n))
    return false;

  for (size_t i (0); i != n; ++i)
    if (o[i] != b[i])
      return false;

  return true;
} ());

// A styled round trip.
//
static_assert ([] ()
{
  const byte b[4] {byte {0xde}, byte {0xad}, byte {0xbe}, byte {0xef}};
  const hex_style s (style (hex_style_id::colon, hex_case::upper));

  char c[11] {};

  if (!to_hex_chars (c, c + sizeof (c), b, 4, s))
    return false;

  if (string_view (c, sizeof (c)) != "DE:AD:BE:EF")
    return false;

  byte o[4] {};
  return from_hex_chars (c, c + sizeof (c), o, 4, s) && o[3] == byte {0xef};
} ());

// Integers.
//
static_assert ([] ()
{
  char c[16] {};

  if (to_hex_chars (c, c + 8, uint32_t (0x0012abcd)).ptr != c + 8)
    return false;

  if (string_view (c, 8) != "0012abcd")
    return false;

  if (to_hex_chars_minimal (c, c + 16, uint32_t (0x0012abcd)).ptr != c + 6)
    return false;

  if (string_view (c, 6) != "12abcd")
    return false;

  // Zero still has a digit.
  //
  if (to_hex_chars_minimal (c, c + 16, uint32_t (0)).ptr != c + 1 || c[0] != '0')
    return false;

  uint64_t v (0);
  const char* s ("ffffffffffffffff");

  return from_hex_chars (s, s + 16, v) && v == 0xffffffffffffffffull;
} ());

static_assert (hex_chars (uint16_t (0xbeef)).size () == 4);
static_assert (hex_chars (uint16_t (0xbeef))[0] == 'b');
static_assert (hex_chars (uint8_t (0x0a), hex_case::upper)[1] == 'A');

namespace
{
  [[noreturn]] void
  fail (const string& m)
  {
    cerr << "error: " << m << endl;
    exit (1);
  }

  class arguments
  {
  public:
    arguments (int argc, char* argv[], int i)
        : argv_ (argv), argc_ (argc), i_ (i) {}

    // The next option, or the empty string once the options are exhausted.
    //
    string
    option ()
    {
      if (i_ == argc_)
        return string ();

      const string o (argv_[i_]);

      if (o.size () < 2 || o[0] != '-')
        return string ();

      ++i_;
      return o;
    }

    // The value of the option just returned.
    //
    string
    value (const string& o)
    {
      if (i_ == argc_)
        fail (o + " requires a value");

      return string (argv_[i_++]);
    }

    // The remaining non-option arguments.
    //
    vector<string>
    rest ()
    {
      vector<string> r;

      for (; i_ != argc_; ++i_)
        r.push_back (argv_[i_]);

      return r;
    }

  private:
    char** argv_;
    int    argc_;
    int    i_;
  };

  optional<hex_style_id>
  style_id (const string& n) noexcept
  {
    for (size_t i (0); i != hex_style_count; ++i)
      if (n == hex_style_table[i].name)
        return static_cast<hex_style_id> (i);

    return nullopt;
  }

  optional<hex_dump_style>
  dump_style (const string& n) noexcept
  {
    for (size_t i (0); i != hex_dump_style_count; ++i)
      if (n == hex_dump_style_table[i].name)
        return static_cast<hex_dump_style> (i);

    return nullopt;
  }

  size_t
  number (const string& s)
  {
    try
    {
      size_t e;
      const unsigned long long v (stoull (s, &e, 0));

      if (e != s.size ())
        throw invalid_argument ("");

      return static_cast<size_t> (v);
    }
    catch (const exception&)
    {
      fail ("invalid number '" + s + "'");
    }
  }

  bool
  style_option (const string& o, arguments& a, hex_style& s)
  {
    if (o == "-s")
    {
      const string n (a.value (o));

      if (const optional<hex_style_id> i = style_id (n))
        s = style (*i, s.digit_case);
      else
        fail ("unknown style '" + n + "'");
    }
    else if (o == "-u")
      s.digit_case = hex_case::upper;
    else if (o == "-c")
    {
      const string v (a.value (o));

      if (v.size () > 1)
        fail ("separator must be a single character");

      s.separator = v.empty () ? '\0' : v[0];
    }
    else if (o == "-g")
      s.group = static_cast<uint8_t> (number (a.value (o)));
    else if (o == "-x")
      s.prefix = true;
    else
      return false;

    return true;
  }

  void
  binary_streams ()
  {
#ifdef _WIN32
    _setmode (_fileno (stdin),  _O_BINARY);
    _setmode (_fileno (stdout), _O_BINARY);
#endif
  }

  vector<char>
  read_octets ()
  {
    cin.exceptions (ios::badbit);

    return vector<char> (istreambuf_iterator<char> (cin),
                         istreambuf_iterator<char> ());
  }

  string
  read_text ()
  {
    const vector<char> b (read_octets ());
    string r (b.begin (), b.end ());

    if (!r.empty () && r.back () == '\n')
      r.pop_back ();

    return r;
  }

  void
  write_octets (span<const byte> b)
  {
    cout.write (reinterpret_cast<const char*> (b.data ()),
                static_cast<streamsize> (b.size ()));
  }
}

template <hex_unsigned T>
static string
encode_integer (uint64_t v, bool minimal, hex_case c)
{
  if (v > static_cast<uint64_t> (numeric_limits<T>::max ()))
    fail ("value does not fit in the selected width");

  const T t (static_cast<T> (v));

  char b[sizeof (T) * 2];

  const hex_to_chars_result r (minimal
                               ? to_hex_chars_minimal (b, b + sizeof (b), t, c)
                               : to_hex_chars (b, b + sizeof (b), t, c));

  assert (r);

  return string (b, r.ptr);
}

template <hex_unsigned T>
static uint64_t
decode_integer (const string& s)
{
  T v (0);

  const hex_from_chars_result r (
    from_hex_chars (s.data (), s.data () + s.size (), v));

  if (!r)
    fail (r.ec == errc::result_out_of_range
          ? "value does not fit in the selected width"
          : "invalid hexadecimal number '" + s + "'");

  if (r.ptr != s.data () + s.size ())
    fail ("trailing characters in '" + s + "'");

  return v;
}

static void
verify ()
{
  const auto reference = [] (span<const uint8_t> b, const hex_style& s)
  {
    const char* d (s.digit_case == hex_case::upper ? "0123456789ABCDEF"
                                                   : "0123456789abcdef");

    string r;

    if (s.prefix)
      r += "0x";

    for (size_t i (0); i != b.size (); ++i)
    {
      if (i != 0 && s.separator != '\0' && i % s.group == 0)
        r += s.separator;

      r += d[b[i] >> 4];
      r += d[b[i] & 0x0f];
    }

    return r;
  };

  // Digits and values, over the whole character set.
  //
  for (unsigned i (0); i != 256; ++i)
  {
    const char c (static_cast<char> (i));
    const uint8_t v (hex_value (c));

    const bool d ((i >= '0' && i <= '9') ||
                  (i >= 'a' && i <= 'f') ||
                  (i >= 'A' && i <= 'F'));

    assert (hex_digit_p (c) == d);
    assert ((v != hex_invalid) == d);

    if (d)
    {
      assert (v < 16);
      assert (hex_digit (v) == (i >= 'A' && i <= 'F'
                                ? static_cast<char> (i - 'A' + 'a')
                                : c));
      assert (hex_nibble (c) == v);
    }
    else
      assert (!hex_nibble (c));
  }

  for (unsigned n (0); n != 16; ++n)
  {
    assert (hex_value (hex_digit (n)) == n);
    assert (hex_value (hex_digit (n, hex_case::upper)) == n);
  }

  // Test every style against the reference, over every length up to well past
  // the point where the word-at-a-time path takes over.
  //
  {
    mt19937_64 e (20260908);

    for (size_t n (0); n != 200; ++n)
    {
      vector<uint8_t> b (n);

      for (uint8_t& x: b)
        x = static_cast<uint8_t> (e () & 0xff);

      for (const hex_case c: {hex_case::lower, hex_case::upper})
      {
        for (size_t i (0); i != hex_style_count; ++i)
        {
          const hex_style s (style (static_cast<hex_style_id> (i), c));
          const string x (reference (b, s));

          assert (hex_encoded_size (n, s) == x.size ());

          string r (x.size (), '\0');
          const hex_to_chars_result t (
            to_hex_chars (r.data (), r.data () + r.size (), b, s));

          assert (t);
          assert (t.ptr == r.data () + r.size ());
          assert (r == x);

          // And the allocating layer agrees.
          //
          assert (hex_encode (b, s) == x);

          vector<uint8_t> o (n, 0xaa);
          const hex_from_chars_result f (
            from_hex_chars (r.data (), r.data () + r.size (), o, s));

          assert (f);
          assert (f.ptr == r.data () + r.size ());
          assert (o == b);

          // The allocating decoder recovers the octet count on its own.
          //
          const optional<vector<byte>> d (try_hex_decode (x, s));
          assert (d);
          assert (d->size () == n);

          if (n != 0)
            assert (memcmp (d->data (), b.data (), n) == 0);
        }
      }
    }
  }

  // Test a group wider than one octet, over every length.
  //
  {
    const hex_style s {hex_case::lower, ' ', 2, false};

    assert (hex_encode (vector<uint8_t> {0x01, 0x23, 0x45, 0x67, 0x89}, s) ==
            "0123 4567 89");

    for (size_t n (0); n != 64; ++n)
    {
      const vector<uint8_t> v (n, 0x5a);
      const string e (hex_encode (v, s));
      const optional<vector<byte>> o (try_hex_decode (e, s));

      assert (o && o->size () == n);
    }
  }

  // Every octet value in every lane of a word.
  //
  for (size_t p (0); p != 8; ++p)
  {
    for (unsigned v (0); v != 256; ++v)
    {
      uint8_t b[8] {};
      b[p] = static_cast<uint8_t> (v);

      char c[16];
      assert (to_hex_chars (c, c + 16, b, 8));
      assert (string (c, 16) == reference (b, hex_style ()));

      uint8_t o[8] {};
      assert (from_hex_chars (c, c + 16, o, 8));
      assert (memcmp (o, b, 8) == 0);
    }
  }

  // Every character in every position, valid or not.
  //
  for (size_t p (0); p != 8; ++p)
  {
    for (unsigned v (0); v != 256; ++v)
    {
      char c[16];
      memset (c, '0', sizeof (c));
      c[p] = static_cast<char> (v);

      uint8_t o[8] {};
      const bool ok (from_hex_chars (c, c + 16, o, 8));

      assert (ok == hex_digit_p (static_cast<char> (v)));

      if (ok)
      {
        uint8_t r[8] {};
        r[p / 2] = static_cast<uint8_t> (
          p % 2 == 0 ? hex_value (static_cast<char> (v)) << 4
                     : hex_value (static_cast<char> (v)));

        assert (memcmp (o, r, 8) == 0);
      }
    }
  }

  // Test a bad character beyond the first four octets.
  //
  {
    const char* s ("00000000000000zz0000");
    uint8_t o[10] {};

    assert (!from_hex_chars (s, s + 20, o, 10));
  }

  // Test buffer handling: too small is an error and writes nothing, and
  // trailing input is not an error.
  //
  {
    const uint8_t b[4] {0xde, 0xad, 0xbe, 0xef};

    char c[16];
    memset (c, '#', sizeof (c));

    const hex_to_chars_result t (to_hex_chars (c, c + 7, b, 4));
    assert (!t);
    assert (t.ec == errc::value_too_large);
    assert (t.ptr == c + 7);

    for (char x: c)
      assert (x == '#');

    assert (to_hex_chars (c, c + 8, b, 4).ptr == c + 8);
    assert (string (c, 8) == "deadbeef");

    uint8_t o[4] {};
    assert (!from_hex_chars (c, c + 7, o, 4));

    const char* l ("deadbeefTRAILING");
    const hex_from_chars_result f (from_hex_chars (l, l + 16, o, 4));
    assert (f);
    assert (f.ptr == l + 8);
  }

  // Test integers over a decent sample.
  //
  {
    mt19937_64 e (7);
    char c[32];

    for (size_t i (0); i != 10000; ++i)
    {
      const uint64_t v (e ());

      assert (to_hex_chars (c, c + 32, v));

      uint64_t o (0);
      const hex_from_chars_result f (from_hex_chars (c, c + 16, o));

      assert (f && f.ptr == c + 16 && o == v);

      const hex_to_chars_result m (to_hex_chars_minimal (c, c + 32, v));
      uint64_t p (0);

      assert (from_hex_chars (c, m.ptr, p));
      assert (p == v);
    }

    // Fixed width sorts.
    //
    to_hex_chars (c, c + 32, uint64_t (1));
    const string one (c, 16);

    to_hex_chars (c, c + 32, uint64_t (2));
    const string two (c, 16);

    assert (one < two);

    to_hex_chars_minimal (c, c + 32, uint64_t (0x10));
    const string m1 (c, 2);

    to_hex_chars_minimal (c, c + 32, uint64_t (0x9));
    const string m2 (c, 1);

    assert (m1 < m2); // 0x10 > 0x9, but "10" < "9".

    // The output is untouched when the input does not fit or has no digits.
    //
    uint16_t v (0xffff);
    const char* o ("12345");

    assert (from_hex_chars (o, o + 5, v).ec == errc::result_out_of_range);
    assert (v == 0xffff);

    const char* z ("zz");
    assert (from_hex_chars (z, z + 2, v).ec == errc::invalid_argument);
    assert (v == 0xffff);
  }

  // Test the allocation-free spellings.
  //
  {
    const hex_integer_chars<uint32_t> h (hex_chars (uint32_t (0xdeadbeef)));
    assert (h.size () == 8);
    assert (string (h.begin (), h.end ()) == "deadbeef");

    const array<byte, 4> a {byte {0xde}, byte {0xad}, byte {0xbe}, byte {0xef}};
    const hex_octet_chars<4> c (hex_chars (span<const byte, 4> (a)));
    assert (string (c.begin (), c.end ()) == "deadbeef");
  }

  // Test that a view keeps its own style unless the format specification
  // overrides it.
  //
  {
    const uint8_t b[4] {0xde, 0xad, 0xbe, 0xef};

    assert (format ("{}", hex (b)) == "deadbeef");
    assert (format ("{}", hex (b, hex_style_id::colon)) == "de:ad:be:ef");
    assert (format ("{:n}", hex (b, hex_style_id::colon)) == "deadbeef");

    try
    {
      hex_view v (hex (b));
      (void) vformat ("{:q}", make_format_args (v));
      assert (false);
    }
    catch (const format_error&) {}

    // Strings are octet ranges too.
    //
    assert (hex_encode (string ("AB")) == "4142");
    assert (hex_encode (string_view ("AB")) == "4142");
    assert (hex_encode (vector<char> {'A', 'B'}) == "4142");
  }

  // Test that both ways of reaching a layout agree.
  //
  {
    const vector<uint8_t> b (20, 0x5a);

    for (size_t i (0); i != hex_dump_style_count; ++i)
    {
      const hex_dump_style s (static_cast<hex_dump_style> (i));

      ostringstream x;
      hex_dumper_for (s).dump (x, as_bytes (span (b)));

      ostringstream y;
      make_hex_dumper (s)->dump (y, as_bytes (span (b)));

      assert (x.str () == y.str ());
      assert (!x.str ().empty () && x.str ().back () == '\n');
    }

    const auto go = []<hex_dump_layout D> (const D& l)
    {
      ostringstream x;
      l.dump (x, span<const byte> ());
      return x.str ();
    };

    assert (go (plain_hex_dumper ()).empty ());

    // The options object is copyable.
    //
    hex_dump_options o;
    o.annotate = [] (size_t f) {return f == 0 ? "first" : string ();};

    const hex_dump_options c (o);
    assert (hex_dump (b, c) == hex_dump (b, o));
  }
}

int
main (int argc, char* argv[])
try
{
  if (argc < 2)
    fail ("missing mode");

  const string m (argv[1]);
  arguments a (argc, argv, 2);

  cout.exceptions (ios::failbit | ios::badbit);

  if (m == "verify")
  {
    verify ();
    return 0;
  }

  if (m == "styles")
  {
    for (const hex_style_traits& t: hex_style_table)
      cout << t.name << ' '
           << t.specifier << ' '
           << (t.style.separator == '\0'
               ? string ("none")
               : "'" + string (1, t.style.separator) + "'") << ' '
           << static_cast<unsigned> (t.style.group) << ' '
           << (t.style.prefix ? "0x" : "none") << '\n';

    return 0;
  }

  if (m == "encode" || m == "decode" || m == "size")
  {
    hex_style s;
    bool relaxed (false);

    for (string o (a.option ()); !o.empty (); o = a.option ())
    {
      if (m == "decode" && o == "-r")
        relaxed = true;
      else if (!style_option (o, a, s))
        fail ("unknown option '" + o + "'");
    }

    if (!s.valid ())
      fail ("a separator requires a non-zero group size");

    if (m == "size")
    {
      const vector<string> r (a.rest ());

      if (r.size () != 1)
        fail ("size expects exactly one octet count");

      cout << hex_encoded_size (number (r[0]), s) << '\n';
      return 0;
    }

    binary_streams ();

    if (m == "encode")
    {
      const vector<char> b (read_octets ());
      cout << hex_encode (b, s) << '\n';
      return 0;
    }

    const string t (read_text ());

    const optional<vector<byte>> d (relaxed
                                    ? try_hex_decode_relaxed (t)
                                    : try_hex_decode (t, s));

    if (!d)
      fail ("invalid hexadecimal representation");

    write_octets (*d);
    return 0;
  }

  if (m == "integer")
  {
    unsigned bits (64);
    bool minimal (false);
    bool dec (false);
    hex_case c (hex_case::lower);

    for (string o (a.option ()); !o.empty (); o = a.option ())
    {
      if (o == "-b")
      {
        bits = static_cast<unsigned> (number (a.value (o)));

        if (bits != 8 && bits != 16 && bits != 32 && bits != 64)
          fail ("width must be 8, 16, 32 or 64");
      }
      else if (o == "-m")
        minimal = true;
      else if (o == "-d")
        dec = true;
      else if (o == "-u")
        c = hex_case::upper;
      else
        fail ("unknown option '" + o + "'");
    }

    cin.exceptions (ios::badbit);

    for (string l; getline (cin, l); )
    {
      if (dec)
      {
        const uint64_t v (bits == 8  ? decode_integer<uint8_t>  (l) :
                          bits == 16 ? decode_integer<uint16_t> (l) :
                          bits == 32 ? decode_integer<uint32_t> (l) :
                                       decode_integer<uint64_t> (l));

        cout << v << '\n';
      }
      else
      {
        const uint64_t v (number (l));

        cout << (bits == 8  ? encode_integer<uint8_t>  (v, minimal, c) :
                 bits == 16 ? encode_integer<uint16_t> (v, minimal, c) :
                 bits == 32 ? encode_integer<uint32_t> (v, minimal, c) :
                              encode_integer<uint64_t> (v, minimal, c))
             << '\n';
      }
    }

    return 0;
  }

  if (m == "dump")
  {
    hex_dump_style l (hex_dump_style::canonical);
    hex_dump_options o;
    bool note (false);

    for (string p (a.option ()); !p.empty (); p = a.option ())
    {
      if (p == "-l")
      {
        const string n (a.value (p));

        if (const optional<hex_dump_style> s = dump_style (n))
          l = *s;
        else
          fail ("unknown layout '" + n + "'");
      }
      else if (p == "-w")
        o.columns = static_cast<uint16_t> (number (a.value (p)));
      else if (p == "-g")
        o.group = static_cast<uint8_t> (number (a.value (p)));
      else if (p == "-O")
        o.offset = number (a.value (p));
      else if (p == "-N")
        o.offsets = false;
      else if (p == "-A")
        o.ascii = false;
      else if (p == "-u")
        o.digit_case = hex_case::upper;
      else if (p == "-a")
        note = true;
      else
        fail ("unknown option '" + p + "'");
    }

    if (!o.valid ())
      fail ("a dump needs at least one column");

    if (note)
    {
      const size_t f (o.offset);
      o.annotate = [f] (size_t x) {return x == f ? "first" : string ();};
    }

    binary_streams ();

    const vector<char> b (read_octets ());
    hex_dumper_for (l).dump (cout, as_bytes (span (b)), o);
    return 0;
  }

  if (m == "format")
  {
    binary_streams ();

    const vector<char> b (read_octets ());
    const hex_view v (hex (b));

    // Driven by the style table.
    //
    for (const hex_style_traits& t: hex_style_table)
    {
      for (const char c: {t.specifier,
                          static_cast<char> (t.specifier - 'a' + 'A')})
      {
        const string s (1, c);

        cout << s << ' '
             << vformat ("{:" + s + "}", make_format_args (v)) << '\n';
      }
    }

    // And the empty specification.
    //
    cout << ". " << format ("{}", v) << '\n';
    return 0;
  }

  fail ("unknown mode '" + m + "'");
}
catch (const std::exception& e)
{
  cerr << "error: " << e.what () << endl;
  return 1;
}
