#pragma once

#include <algorithm>    // min(), ranges::copy()
#include <array>
#include <bit>          // byteswap(), countl_zero(), endian
#include <concepts>
#include <cstddef>      // size_t, byte
#include <cstdint>
#include <cstring>      // memcpy()
#include <format>
#include <functional>   // copyable_function
#include <inplace_vector>
#include <iosfwd>
#include <limits>
#include <memory>       // unique_ptr
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <system_error> // errc
#include <type_traits>
#include <utility>      // to_underlying()
#include <vector>

#include <libiw4x/export.hxx>
#include <libiw4x/contract.hxx>

namespace iw4x
{
  enum class hex_case: unsigned char
  {
    lower, // 0123456789abcdef
    upper  // 0123456789ABCDEF
  };

  inline constexpr std::size_t hex_case_count = 2;

  constexpr const char*
  hex_case_name (hex_case c) noexcept
  {
    switch (c)
    {
      case hex_case::lower: return "lower";
      case hex_case::upper: return "upper";
    }

    LIBIW4X_UNREACHABLE ();
  }

  template <typename B>
  concept hex_byte =
    std::same_as<std::remove_cv_t<B>, std::byte>         ||
    std::same_as<std::remove_cv_t<B>, unsigned char>     ||
    std::same_as<std::remove_cv_t<B>, signed char>       ||
    std::same_as<std::remove_cv_t<B>, char>;

  template <typename R>
  concept hex_byte_range =
    std::ranges::contiguous_range<R> &&
    std::ranges::sized_range<R>      &&
    hex_byte<
      std::remove_cvref_t<
        std::ranges::range_reference_t<R>
      >
    >;

  template <typename R>
  concept hex_mutable_byte_range =
    hex_byte_range<R> &&
    !std::is_const_v<
      std::remove_reference_t<
        std::ranges::range_reference_t<R>
      >
    >;

  template <hex_byte_range R>
  using hex_range_byte = std::remove_reference_t<
    std::ranges::range_reference_t<R>
  >;

  template <typename T>
  concept hex_unsigned =
    std::unsigned_integral<T> && !std::same_as<std::remove_cv_t<T>, bool>;

  template <hex_byte B>
  constexpr std::uint8_t
  hex_octet (B b) noexcept
  {
    return static_cast<std::uint8_t> (b);
  }

  template <hex_byte B>
  constexpr std::remove_cv_t<B>
  hex_from_octet (std::uint8_t v) noexcept
  {
    return static_cast<std::remove_cv_t<B>> (v);
  }

  inline constexpr std::uint8_t hex_invalid = 0xff;

  constexpr bool
  hex_accumulated_invalid (std::uint8_t b) noexcept
  {
    return (b & 0xf0u) != 0;
  }

  namespace detail
  {
    consteval std::array<char, 16>
    make_hex_alphabet (hex_case c) noexcept
    {
      std::array<char, 16> r {};

      for (std::size_t i (0); i != 16; ++i)
        r[i] = static_cast<char> (i < 10
                                  ? '0' + i
                                  : (c == hex_case::upper
                                     ? 'A'
                                     : 'a') + (i - 10));

      return r;
    }

    consteval std::array<std::uint8_t, 256>
    make_hex_values () noexcept
    {
      std::array<std::uint8_t, 256> r {};

      r.fill (hex_invalid);

      for (unsigned c ('0'); c <= '9'; ++c)
        r[c] = static_cast<std::uint8_t> (c - '0');

      for (unsigned c ('a'); c <= 'f'; ++c)
        r[c] = static_cast<std::uint8_t> (c - 'a' + 10);

      for (unsigned c ('A'); c <= 'F'; ++c)
        r[c] = static_cast<std::uint8_t> (c - 'A' + 10);

      return r;
    }

    inline constexpr std::array<char, 16> hex_alphabet_lower =
      make_hex_alphabet (hex_case::lower);

    inline constexpr std::array<char, 16> hex_alphabet_upper =
      make_hex_alphabet (hex_case::upper);

    inline constexpr std::array<std::uint8_t, 256> hex_value_table =
      make_hex_values ();
  }

  constexpr char
  hex_digit (unsigned n, hex_case c = hex_case::lower) noexcept
  {
    LIBIW4X_PRE (n < 16);

    return c == hex_case::upper ? detail::hex_alphabet_upper[n]
                                : detail::hex_alphabet_lower[n];
  }

  constexpr std::uint8_t
  hex_value (char c) noexcept
  {
    return detail::hex_value_table[static_cast<unsigned char> (c)];
  }

  constexpr bool
  hex_digit_p (char c) noexcept
  {
    return hex_value (c) != hex_invalid;
  }

  constexpr std::optional<std::uint8_t>
  hex_nibble (char c) noexcept
  {
    const std::uint8_t v (hex_value (c));

    if (v == hex_invalid)
      return std::nullopt;

    return v;
  }

  struct hex_style
  {
    hex_case     digit_case = hex_case::lower;
    char         separator  = '\0';  // Between groups, '\0' for none.
    std::uint8_t group      = 1;     // Octets per group.
    bool         prefix     = false; // Leading "0x".

    constexpr bool
    valid () const noexcept
    {
      return separator == '\0' || group != 0;
    }

    friend constexpr bool
    operator== (const hex_style&, const hex_style&) noexcept = default;
  };

  static_assert (sizeof (hex_style) == 4,
                 "hex_style must not contain padding");

  static_assert (std::is_trivially_copyable_v<hex_style>);
  static_assert (std::is_aggregate_v<hex_style>);

  enum class hex_style_id: unsigned char
  {
    compact,  // deadbeef
    spaced,   // de ad be ef
    colon,    // de:ad:be:ef
    dashed,   // de-ad-be-ef
    prefixed  // 0xdeadbeef
  };

  inline constexpr std::size_t hex_style_count = 5;

  struct hex_style_traits
  {
    const char* name;      // Spelled like the enumerator.
    hex_style   style;
    char        specifier; // std::format() specifier letter, lower case.
  };

  static_assert (sizeof (hex_style_traits) == 16,
                 "hex_style_traits must fit in 16 bytes");

  inline constexpr hex_style_traits hex_style_table[hex_style_count] =
  {
    /* compact  */ {"compact",  {hex_case::lower, '\0', 1, false}, 'n'},
    /* spaced   */ {"spaced",   {hex_case::lower, ' ',  1, false}, 's'},
    /* colon    */ {"colon",    {hex_case::lower, ':',  1, false}, 'c'},
    /* dashed   */ {"dashed",   {hex_case::lower, '-',  1, false}, 'd'},
    /* prefixed */ {"prefixed", {hex_case::lower, '\0', 1, true},  'x'}
  };

  constexpr const hex_style_traits&
  traits (hex_style_id i) noexcept
  {
    LIBIW4X_PRE (static_cast<std::size_t> (i) < hex_style_count);

    return hex_style_table[static_cast<std::size_t> (i)];
  }

  constexpr hex_style
  style (hex_style_id i, hex_case c = hex_case::lower) noexcept
  {
    hex_style r (traits (i).style);
    r.digit_case = c;

    LIBIW4X_ASSERT (r.valid ());

    return r;
  }

  constexpr const char*
  hex_style_name (hex_style_id i) noexcept
  {
    return traits (i).name;
  }

  constexpr std::size_t
  hex_encoded_size (std::size_t n, const hex_style& s = hex_style ()) noexcept
  {
    LIBIW4X_PRE (s.valid ());
    LIBIW4X_PRE (n <= (std::numeric_limits<std::size_t>::max () - 2) / 3);

    std::size_t r (n * 2);

    if (s.separator != '\0' && n > 1)
      r += (n - 1) / s.group;

    if (s.prefix)
      r += 2;

    return r;
  }

  constexpr std::size_t
  hex_decoded_size (std::size_t n) noexcept
  {
    return n / 2;
  }

  namespace detail
  {
    inline constexpr std::uint64_t hex_ones = 0x0101010101010101ull;
    inline constexpr std::uint64_t hex_high = 0x8080808080808080ull;

    constexpr std::uint64_t
    hex_alpha_bias (hex_case c) noexcept
    {
      return c == hex_case::upper ? 7u : 39u;
    }

    constexpr std::uint64_t
    hex_spread (std::uint32_t v, std::uint64_t alpha) noexcept
    {
      std::uint64_t x (v);

      // We start with four packed octets in the low 32 bits. The
      // desired result has eight byte lanes, one lane per hexadecimal
      // digit, so first give each source octet its own 16-bit lane.
      //
      x = ((x & 0xffff0000ull) << 16)        | (x & 0x0000ffffull);
      x = ((x & 0x0000ff000000ff00ull) << 8) | (x & 0x000000ff000000ffull);

      // Each 16-bit lane now contains one source octet. Split its two
      // nibbles into adjacent byte lanes. The high nibble is placed
      // first so storing the resulting word produces the usual
      // hexadecimal order.
      //
      x = ((x & 0x000f000f000f000full) << 8)
        | ((x & 0x00f000f000f000f0ull) >> 4);

      // Now each byte lane contains a value from 0 through 15. Adding
      // six sets bit 4 exactly for values 10 through 15. Shifting that
      // bit down gives a one-byte multiplier for the alphabetic
      // adjustment.
      //
      // Note that every lane remains independent. The largest final
      // value is the selected representation of nibble 15, so no carry
      // can escape into the neighboring lane.
      //
      const std::uint64_t m (((x + 0x0606060606060606ull) >> 4) & hex_ones);

      return x + 0x3030303030303030ull + m * alpha;
    }

    static_assert (std::endian::native == std::endian::little ||
                   std::endian::native == std::endian::big,
                   "mixed-endian targets are not supported");

    template <hex_byte B>
    inline std::uint32_t
    hex_load4 (const B* b) noexcept
    {
      std::uint32_t v;
      std::memcpy (&v, b, 4);

      if constexpr (std::endian::native == std::endian::big)
        v = std::byteswap (v);

      return v;
    }

    inline void
    hex_store8 (char* p, std::uint64_t w) noexcept
    {
      if constexpr (std::endian::native == std::endian::big)
        w = std::byteswap (w);

      std::memcpy (p, &w, 8);
    }
  }

  template <hex_byte B>
  constexpr char*
  hex_encode_to (char* p,
                 const B* b,
                 std::size_t n,
                 hex_case c = hex_case::lower) noexcept
  {
    LIBIW4X_PRE (p != nullptr || n == 0);
    LIBIW4X_PRE (b != nullptr || n == 0);

    const std::array<char, 16>& a (c == hex_case::upper
                                   ? detail::hex_alphabet_upper
                                   : detail::hex_alphabet_lower);

    std::size_t i (0);

    if !consteval
    {
      const std::uint64_t alpha (detail::hex_alpha_bias (c));

      for (; i + 4 <= n; i += 4)
        detail::hex_store8 (p + i * 2,
                            detail::hex_spread (detail::hex_load4 (b + i),
                                                alpha));
    }

    for (; i != n; ++i)
    {
      const std::uint8_t v (hex_octet (b[i]));

      p[i * 2]     = a[v >> 4];
      p[i * 2 + 1] = a[v & 0x0f];
    }

    return p + n * 2;
  }

  template <hex_byte B>
  constexpr const char*
  hex_decode_from (const char* p,
                   B* b,
                   std::size_t n,
                   std::uint8_t& bad) noexcept
    requires (!std::is_const_v<B>)
  {
    LIBIW4X_PRE (p != nullptr || n == 0);
    LIBIW4X_PRE (b != nullptr || n == 0);

    for (std::size_t i (0); i != n; ++i)
    {
      const std::uint8_t h (hex_value (p[i * 2]));
      const std::uint8_t l (hex_value (p[i * 2 + 1]));

      bad = static_cast<std::uint8_t> (bad | h | l);

      b[i] = hex_from_octet<B> (static_cast<std::uint8_t> (
                                  (h << 4) | (l & 0x0f)));
    }

    return p + n * 2;
  }

  struct hex_to_chars_result
  {
    char*     ptr;
    std::errc ec;

    constexpr explicit
    operator bool () const noexcept {return ec == std::errc ();}
  };

  struct hex_from_chars_result
  {
    const char* ptr;
    std::errc   ec;

    constexpr explicit
    operator bool () const noexcept {return ec == std::errc ();}
  };

  template <hex_byte B>
  constexpr hex_to_chars_result
  to_hex_chars (char* first,
                char* last,
                const B* b,
                std::size_t n,
                const hex_style& s = hex_style ()) noexcept
  {
    LIBIW4X_PRE (first <= last);
    LIBIW4X_PRE (s.valid ());

    const std::size_t need (hex_encoded_size (n, s));

    if (static_cast<std::size_t> (last - first) < need)
      return {last, std::errc::value_too_large};

    char* p (first);

    if (s.prefix)
    {
      p[0] = '0';
      p[1] = 'x';
      p += 2;
    }

    if (s.separator == '\0')
      p = hex_encode_to (p, b, n, s.digit_case);
    else
    {
      for (std::size_t i (0); i != n; )
      {
        const std::size_t k (std::min<std::size_t> (s.group, n - i));

        p = hex_encode_to (p, b + i, k, s.digit_case);
        i += k;

        if (i != n)
          *p++ = s.separator;
      }
    }

    LIBIW4X_ASSERT (p == first + need);

    return {p, std::errc ()};
  }

  template <hex_byte_range R>
  constexpr hex_to_chars_result
  to_hex_chars (char* first,
                char* last,
                const R& r,
                const hex_style& s = hex_style ()) noexcept
  {
    return to_hex_chars (first,
                         last,
                         std::ranges::data (r),
                         std::ranges::size (r),
                         s);
  }

  template <hex_byte B>
  constexpr hex_from_chars_result
  from_hex_chars (const char* first,
                  const char* last,
                  B* b,
                  std::size_t n,
                  const hex_style& s = hex_style ()) noexcept
    requires (!std::is_const_v<B>)
  {
    LIBIW4X_PRE (first <= last);
    LIBIW4X_PRE (s.valid ());

    const std::size_t need (hex_encoded_size (n, s));

    if (static_cast<std::size_t> (last - first) < need)
      return {first, std::errc::invalid_argument};

    const char* p (first);

    if (s.prefix)
    {
      if (p[0] != '0' || (p[1] != 'x' && p[1] != 'X'))
        return {p, std::errc::invalid_argument};

      p += 2;
    }

    std::uint8_t bad (0);

    if (s.separator == '\0')
      p = hex_decode_from (p, b, n, bad);
    else
    {
      for (std::size_t i (0); i != n; )
      {
        const std::size_t k (std::min<std::size_t> (s.group, n - i));

        p = hex_decode_from (p, b + i, k, bad);
        i += k;

        if (i != n && *p++ != s.separator)
          return {p - 1, std::errc::invalid_argument};
      }
    }

    if (hex_accumulated_invalid (bad))
      return {first, std::errc::invalid_argument};

    LIBIW4X_ASSERT (p == first + need);

    return {p, std::errc ()};
  }

  template <hex_mutable_byte_range R>
  constexpr hex_from_chars_result
  from_hex_chars (const char* first,
                  const char* last,
                  R&& r,
                  const hex_style& s = hex_style ()) noexcept
  {
    return from_hex_chars (first,
                           last,
                           std::ranges::data (r),
                           std::ranges::size (r),
                           s);
  }

  inline constexpr std::string_view hex_default_ignored = " \t\r\n:-_";

  template <hex_byte B>
  constexpr hex_from_chars_result
  from_hex_chars_relaxed (const char* first,
                          const char* last,
                          B* b,
                          std::size_t n,
                          std::string_view ignored
                            = hex_default_ignored) noexcept
    requires (!std::is_const_v<B>)
  {
    LIBIW4X_PRE (first <= last);

    const char* p (first);

    if (last - p >= 2 && p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
      p += 2;

    for (std::size_t i (0); i != n; ++i)
    {
      while (p != last && ignored.find (*p) != std::string_view::npos)
        ++p;

      if (last - p < 2)
        return {p, std::errc::invalid_argument};

      const std::uint8_t h (hex_value (p[0]));
      const std::uint8_t l (hex_value (p[1]));

      if (hex_accumulated_invalid (static_cast<std::uint8_t> (h | l)))
        return {p, std::errc::invalid_argument};

      b[i] = hex_from_octet<B> (static_cast<std::uint8_t> ((h << 4) | l));
      p += 2;
    }

    return {p, std::errc ()};
  }

  template <hex_mutable_byte_range R>
  constexpr hex_from_chars_result
  from_hex_chars_relaxed (const char* first,
                          const char* last,
                          R&& r,
                          std::string_view ignored
                            = hex_default_ignored) noexcept
  {
    return from_hex_chars_relaxed (first,
                                   last,
                                   std::ranges::data (r),
                                   std::ranges::size (r),
                                   ignored);
  }

  template <hex_unsigned T>
  constexpr hex_to_chars_result
  to_hex_chars (char* first,
                char* last,
                T v,
                hex_case c = hex_case::lower) noexcept
  {
    LIBIW4X_PRE (first <= last);

    constexpr std::size_t n (sizeof (T) * 2);

    if (static_cast<std::size_t> (last - first) < n)
      return {last, std::errc::value_too_large};

    const std::array<char, 16>& a (c == hex_case::upper
                                   ? detail::hex_alphabet_upper
                                   : detail::hex_alphabet_lower);

    for (std::size_t i (0); i != n; ++i)
      first[i] = a[(v >> ((n - 1 - i) * 4)) & 0x0f];

    return {first + n, std::errc ()};
  }

  template <hex_unsigned T>
  constexpr hex_to_chars_result
  to_hex_chars_minimal (char* first,
                        char* last,
                        T v,
                        hex_case c = hex_case::lower) noexcept
  {
    LIBIW4X_PRE (first <= last);

    const std::size_t n (
      v == 0 ? 1
             : (sizeof (T) * 2 -
                static_cast<std::size_t> (std::countl_zero (v)) / 4));

    LIBIW4X_ASSERT (n >= 1 && n <= sizeof (T) * 2);

    if (static_cast<std::size_t> (last - first) < n)
      return {last, std::errc::value_too_large};

    const std::array<char, 16>& a (c == hex_case::upper
                                   ? detail::hex_alphabet_upper
                                   : detail::hex_alphabet_lower);

    for (std::size_t i (0); i != n; ++i)
      first[i] = a[(v >> ((n - 1 - i) * 4)) & 0x0f];

    return {first + n, std::errc ()};
  }

  template <hex_unsigned T>
  constexpr hex_from_chars_result
  from_hex_chars (const char* first, const char* last, T& v) noexcept
  {
    LIBIW4X_PRE (first <= last);

    constexpr std::size_t max (sizeof (T) * 2);

    const char* p (first);
    T r (0);
    std::size_t n (0);

    for (; p != last; ++p, ++n)
    {
      const std::uint8_t d (hex_value (*p));

      if (d == hex_invalid)
        break;

      if (n == max)
        return {first, std::errc::result_out_of_range};

      r = static_cast<T> ((r << 4) | d);
    }

    if (n == 0)
      return {first, std::errc::invalid_argument};

    v = r;
    return {p, std::errc ()};
  }

  template <hex_unsigned T>
  using hex_integer_chars = std::inplace_vector<char, sizeof (T) * 2>;

  template <hex_unsigned T>
  constexpr hex_integer_chars<T>
  hex_chars (T v, hex_case c = hex_case::lower) noexcept
  {
    hex_integer_chars<T> r;
    r.resize (sizeof (T) * 2);

    const hex_to_chars_result t (
      to_hex_chars (r.data (), r.data () + r.size (), v, c));

    LIBIW4X_ASSERT (t.ec == std::errc ());

    return r;
  }

  template <std::size_t N>
  using hex_octet_chars = std::inplace_vector<char, N * 2>;

  template <std::size_t N, hex_byte B>
  constexpr hex_octet_chars<N>
  hex_chars (std::span<B, N> b, hex_case c = hex_case::lower) noexcept
    requires (N != std::dynamic_extent)
  {
    hex_octet_chars<N> r;
    r.resize (N * 2);

    hex_encode_to (r.data (), b.data (), N, c);

    return r;
  }

  LIBIW4X_SYMEXPORT std::string
  hex_encode (std::span<const std::byte>, const hex_style& = hex_style ());

  template <hex_byte_range R>
  inline std::string
  hex_encode (const R& r, const hex_style& s = hex_style ())
  {
    return hex_encode (std::as_bytes (std::span (std::ranges::data (r),
                                                 std::ranges::size (r))), s);
  }

  LIBIW4X_SYMEXPORT std::vector<std::byte>
  hex_decode (std::string_view, const hex_style& = hex_style ());

  LIBIW4X_SYMEXPORT std::optional<std::vector<std::byte>>
  try_hex_decode (std::string_view, const hex_style& = hex_style ());

  LIBIW4X_SYMEXPORT std::optional<std::vector<std::byte>>
  try_hex_decode_relaxed (std::string_view,
                          std::string_view ignored = hex_default_ignored);

  struct hex_view
  {
    std::span<const std::byte> octets;
    hex_style                  style {};
  };

  template <hex_byte_range R>
  inline hex_view
  hex (const R& r, const hex_style& s = hex_style ()) noexcept
  {
    return hex_view {std::as_bytes (std::span (std::ranges::data (r),
                                               std::ranges::size (r))), s};
  }

  template <hex_byte_range R>
  inline hex_view
  hex (const R& r, hex_style_id i, hex_case c = hex_case::lower) noexcept
  {
    return hex (r, style (i, c));
  }

  LIBIW4X_SYMEXPORT std::ostream&
  operator<< (std::ostream&, const hex_view&);

  struct hex_dump_options
  {
    std::size_t offset = 0;

    std::copyable_function<std::string (std::size_t) const> annotate;

    std::uint16_t columns    = 16;   // Octets per line.
    std::uint8_t  group      = 8;    // Extra space every so many; 0 for none.
    hex_case      digit_case = hex_case::lower;
    bool          offsets    = true; // Show the offset column.
    bool          ascii      = true; // Show the printable column.

    constexpr bool
    valid () const noexcept
    {
      return columns != 0;
    }
  };

  class LIBIW4X_SYMEXPORT hex_dumper
  {
  public:
    virtual
    ~hex_dumper () = default;

    hex_dumper () = default;

    hex_dumper (const hex_dumper&) = delete;
    hex_dumper& operator= (const hex_dumper&) = delete;

    virtual void
    dump (std::ostream&,
          std::span<const std::byte>,
          const hex_dump_options& = {}) const = 0;

    std::string
    dump (std::span<const std::byte>, const hex_dump_options& = {}) const;
  };

  template <typename D>
  concept hex_dump_layout = requires (const D& d,
                                      std::ostream& o,
                                      std::span<const std::byte> b,
                                      const hex_dump_options& p)
  {
    { d.dump (o, b, p) } -> std::same_as<void>;
  };

  enum class hex_dump_style: unsigned char
  {
    canonical, // 00000000  de ad be ef  ...  |....|
    plain,     // deadbeef...
    c_array    // 0xde, 0xad, 0xbe, 0xef,
  };

  inline constexpr std::size_t hex_dump_style_count = 3;

  struct hex_dump_style_traits
  {
    const char* name;
  };

  inline constexpr hex_dump_style_traits
  hex_dump_style_table[hex_dump_style_count] =
  {
    {"canonical"}, {"plain"}, {"c_array"}
  };

  constexpr const char*
  hex_dump_style_name (hex_dump_style s) noexcept
  {
    LIBIW4X_PRE (static_cast<std::size_t> (s) < hex_dump_style_count);

    return hex_dump_style_table[static_cast<std::size_t> (s)].name;
  }

  class LIBIW4X_SYMEXPORT canonical_hex_dumper: public hex_dumper
  {
  public:
    using hex_dumper::dump;

    virtual void
    dump (std::ostream&,
          std::span<const std::byte>,
          const hex_dump_options& = {}) const override;
  };

  class LIBIW4X_SYMEXPORT plain_hex_dumper: public hex_dumper
  {
  public:
    using hex_dumper::dump;

    virtual void
    dump (std::ostream&,
          std::span<const std::byte>,
          const hex_dump_options& = {}) const override;
  };

  class LIBIW4X_SYMEXPORT c_array_hex_dumper: public hex_dumper
  {
  public:
    using hex_dumper::dump;

    virtual void
    dump (std::ostream&,
          std::span<const std::byte>,
          const hex_dump_options& = {}) const override;
  };

  static_assert (hex_dump_layout<canonical_hex_dumper>);
  static_assert (hex_dump_layout<plain_hex_dumper>);
  static_assert (hex_dump_layout<c_array_hex_dumper>);

  LIBIW4X_SYMEXPORT const hex_dumper&
  hex_dumper_for (hex_dump_style) noexcept;

  LIBIW4X_SYMEXPORT std::unique_ptr<hex_dumper>
  make_hex_dumper (hex_dump_style);

  LIBIW4X_SYMEXPORT std::string
  hex_dump (std::span<const std::byte>, const hex_dump_options& = {});

  template <hex_byte_range R>
  inline std::string
  hex_dump (const R& r, const hex_dump_options& o = {})
  {
    return hex_dump (std::as_bytes (std::span (std::ranges::data (r),
                                               std::ranges::size (r))), o);
  }
}

namespace std
{
  template <>
  struct formatter<iw4x::hex_view, char>
  {
    bool               overridden_ = false;
    iw4x::hex_style_id style_      = iw4x::hex_style_id::compact;
    iw4x::hex_case     case_       = iw4x::hex_case::lower;

    constexpr auto
    parse (basic_format_parse_context<char>& ctx)
    {
      auto i (ctx.begin ());

      if (i == ctx.end () || *i == '}')
        return i;

      const char c (*i);
      const char l (c >= 'A' && c <= 'Z' ? static_cast<char> (c - 'A' + 'a')
                                         : c);

      bool found (false);

      for (const iw4x::hex_style_traits& t: iw4x::hex_style_table)
      {
        if (t.specifier == l)
        {
          style_ = static_cast<iw4x::hex_style_id> (
            &t - iw4x::hex_style_table);
          case_ =
            (c == l ? iw4x::hex_case::lower : iw4x::hex_case::upper);
          overridden_ = true;
          found = true;
          break;
        }
      }

      if (!found)
        throw format_error ("invalid hexadecimal format specifier");

      ++i;

      if (i != ctx.end () && *i != '}')
        throw format_error ("invalid hexadecimal format specification");

      return i;
    }

    template <typename O> auto
    format (const iw4x::hex_view& v,
            basic_format_context<O, char>& ctx) const
    {
      const iw4x::hex_style s (overridden_
                               ? iw4x::style (style_, case_)
                               : v.style);

      const string r (iw4x::hex_encode (v.octets, s));

      return ranges::copy (r, ctx.out ()).out;
    }
  };
}
