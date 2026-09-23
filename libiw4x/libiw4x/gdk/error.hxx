#pragma once

#include <cstdint>
#include <cstddef>
#include <concepts>
#include <string_view>

#include <libiw4x/gdk/text.hxx>
#include <libiw4x/gdk/types.hxx>

namespace iw4x
{
  namespace gdk
  {
    inline constexpr std::size_t description_size (192);

    class failure
    {
    public:
      template <writable... A>
      explicit
      failure (HRESULT c, const char* f, const A&... a) noexcept
          : code_ (c), size_ (0)
      {
        text_writer w (what_, description_size);

        format (w, f, a...);

        size_ = static_cast<std::uint32_t> (w.size ());
      }

      HRESULT
      code () const noexcept
      {
        return code_;
      }

      const char*
      what () const noexcept
      {
        return what_;
      }

      template <writable... A>
      void
      append (const char* f, const A&... a) noexcept
      {
        text_writer w (what_ + size_, description_size - size_);

        format (w, f, a...);

        size_ += static_cast<std::uint32_t> (w.size ());
      }

    private:
      HRESULT       code_;
      std::uint32_t size_;
      char          what_[description_size];
    };

    void
    append_last_error (failure&, DWORD) noexcept;

    template <writable... A>
    [[noreturn]] inline void
    raise (HRESULT c, const char* f, const A&... a)
    {
      throw failure (c, f, a...);
    }

    template <writable... A>
    [[noreturn]] inline void
    raise_win32 (const char* f, const A&... a)
    {
      DWORD e (GetLastError ());

      failure r (HRESULT_FROM_WIN32 (e), f, a...);

      append_last_error (r, e);

      throw r;
    }

    template <writable... A>
    [[noreturn]] inline void
    raise_invalid (const char* f, const A&... a)
    {
      throw failure (E_INVALIDARG, f, a...);
    }

    [[noreturn]] void
    fatal (std::string_view) noexcept;

    template <writable... A>
    [[noreturn]] inline void
    fatal (const char* f, const A&... a) noexcept
    {
      fatal (text<description_size> (f, a...));
    }

    HRESULT
    report (const char* entry_point) noexcept;

    void
    note_failure (const char* entry_point, HRESULT) noexcept;

    const char*
    last_failure (HRESULT) noexcept;

    template <typename F, typename R>
    concept body_of = requires (F& f)
    {
      { f () } -> std::same_as<R>;
    };

    template <body_of<HRESULT> F>
    inline HRESULT
    guard (const char* p, F&& body) noexcept
    {
      try
      {
        HRESULT r (body ());

        if (FAILED (r))
          note_failure (p, r);

        return r;
      }
      catch (...)
      {
        return report (p);
      }
    }

    template <body_of<void> F>
    inline void
    guard (const char* p, F&& body) noexcept
    {
      try
      {
        body ();
      }
      catch (...)
      {
        report (p);
      }
    }

    template <typename R, body_of<R> F>
    inline R
    guard (const char* p, R failed, F&& body) noexcept
    {
      try
      {
        return body ();
      }
      catch (...)
      {
        report (p);
        return failed;
      }
    }
  }
}
