#pragma once

#include <cstdint>
#include <cstddef>
#include <concepts>

namespace iw4x
{
  namespace gdk
  {
    class chars
    {
    public:
      constexpr
      chars () noexcept: b_ (""), n_ (0) {}

      explicit constexpr
      chars (const char* b, std::size_t n) noexcept: b_ (b), n_ (n) {}

      explicit constexpr
      chars (const char* b) noexcept: b_ (b), n_ (length (b)) {}

      constexpr const char*
      data () const noexcept
      {
        return b_;
      }

      constexpr std::size_t
      size () const noexcept
      {
        return n_;
      }

      constexpr bool
      empty () const noexcept
      {
        return n_ == 0;
      }

      static constexpr std::size_t
      length (const char* p) noexcept
      {
        std::size_t n (0);

        for (; p[n] != '\0'; ++n)
          ;

        return n;
      }

    private:
      const char* b_;
      std::size_t n_;
    };

    constexpr bool
    operator== (chars x, chars y) noexcept
    {
      if (x.size () != y.size ())
        return false;

      for (std::size_t i (0); i != x.size (); ++i)
        if (x.data ()[i] != y.data ()[i])
          return false;

      return true;
    }

    struct hex
    {
      std::uint64_t value;
      unsigned      width;

      explicit constexpr
      hex (std::uint64_t v, unsigned w = 0) noexcept: value (v), width (w) {}
    };

    class text_writer
    {
    public:
      explicit
      text_writer (char* b, std::size_t c) noexcept: b_ (b), c_ (c), n_ (0)
      {
        terminate ();
      }

      void write (chars) noexcept;
      void write (const char*) noexcept;
      void write (char) noexcept;
      void write (bool) noexcept;
      void write (const void*) noexcept;
      void write (hex) noexcept;

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
      const char* p (f);

      ((p = write_literal (w, p), w.write (a)), ...);

      write_rest (w, p);
    }

    template <std::size_t N>
    class text
    {
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

      operator chars () const noexcept
      {
        return chars (b_, n_);
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
