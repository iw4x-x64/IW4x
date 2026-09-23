#pragma once

#include <limits>
#include <cstdint>
#include <cstddef>
#include <concepts>
#include <string_view>

#include <libiw4x/contract.hxx>

namespace iw4x
{
  namespace gdk
  {
    struct hex_number
    {
      std::uint64_t value;
      unsigned      width;

      explicit constexpr
      hex_number (std::uint64_t v, unsigned w = 0) noexcept
        : value (v), width (w) {}
    };

    class text_writer
    {
    public:
      explicit
      text_writer (char* b, std::size_t c) noexcept: b_ (b), c_ (c), n_ (0)
      {
        LIBIW4X_PRE (b != nullptr && c != 0);

        terminate ();
      }

      void write (std::string_view) noexcept;
      void write (const char*) noexcept;
      void write (char) noexcept;
      void write (bool) noexcept;
      void write (const void*) noexcept;
      void write (hex_number) noexcept;

      template <std::integral T>
      void
      write (T v) noexcept
      {
        if constexpr (std::is_signed_v<T>)
          write_signed (static_cast<std::int64_t> (v));
        else
          write_unsigned (static_cast<std::uint64_t> (v));
      }

      std::size_t
      size () const noexcept
      {
        return n_;
      }

      bool
      whole () const noexcept
      {
        return !truncated_;
      }

    private:
      void write_signed (std::int64_t) noexcept;
      void write_unsigned (std::uint64_t) noexcept;

      void
      terminate () noexcept
      {
        if (c_ != 0)
          b_[n_] = '\0';
      }

      char*       b_;
      std::size_t c_;
      std::size_t n_;
      bool        truncated_ = false;
    };

    template <typename T>
    concept writable = requires (text_writer& w, const T& v)
    {
      w.write (v);
    };

    const char*
    write_literal (text_writer&, const char*) noexcept;

    void
    write_rest (text_writer&, const char*) noexcept;

    template <writable... A>
    inline void
    format (text_writer& w, const char* f, const A&... a) noexcept
    {
      LIBIW4X_PRE (f != nullptr);

      const char* p (f);

      ((p = write_literal (w, p), w.write (a)), ...);

      write_rest (w, p);
    }

    template <std::size_t N>
    class text
    {
      static_assert (N != 0, "room for the terminator");
      static_assert (N <= std::numeric_limits<std::uint32_t>::max (),
                     "a size that fits the recorded length");

    public:
      text () noexcept: n_ (0)
      {
        b_[0] = '\0';
      }

      template <writable... A>
      explicit
      text (const char* f, const A&... a) noexcept
      {
        text_writer w (b_, N);

        format (w, f, a...);

        n_ = static_cast<std::uint32_t> (w.size ());
      }

      const char*
      c_str () const noexcept
      {
        return b_;
      }

      const char*
      data () const noexcept
      {
        return b_;
      }

      operator std::string_view () const noexcept
      {
        return std::string_view (b_, n_);
      }

      std::size_t
      size () const noexcept
      {
        return n_;
      }

    private:
      std::uint32_t n_;
      char          b_[N];
    };
  }
}
