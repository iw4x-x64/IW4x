#pragma once

#include <cstddef>
#include <cstdint>

#include <libiw4x/gdk/text.hxx>

#include <libiw4x/xcurl/xcurl.hxx>
#include <libiw4x/xcurl/local.hxx>

namespace iw4x
{
  namespace xcurl
  {
    inline constexpr std::uint64_t transfer_signature (0x495734582D454153ULL);

    class transfer
    {
    public:
      transfer () noexcept = default;

      bool
      ours () const noexcept
      {
        return signature_ == transfer_signature;
      }

      void
      disown () noexcept
      {
        signature_ = 0;
      }

      transfer (const transfer&) = delete;
      transfer& operator= (const transfer&) = delete;

      bool
      open () noexcept;

      void
      close () noexcept;

      CURL*
      easy () const noexcept
      {
        return easy_;
      }

      CURLcode
      set (int, long) noexcept;

      CURLcode
      set (int, void*) noexcept;

      CURLcode
      set (int, generic_function) noexcept;

      CURLcode
      get (int, long&) const noexcept;

      bool
      begin () noexcept;

      bool
      deliver () noexcept;

      bool
      local () const noexcept
      {
        return local_;
      }

      bool
      delivered () const noexcept
      {
        return delivered_;
      }

      CURLcode
      outcome () const noexcept
      {
        return outcome_;
      }

      gdk::chars
      url () const noexcept
      {
        return url_;
      }

      gdk::chars
      method () const noexcept
      {
        return method_;
      }

    private:
      std::uint64_t signature_ = 0;

      CURL* easy_ = nullptr;

      gdk::text<url_limit>    url_;
      gdk::text<method_limit> method_ {"GET"};

      curl_write_callback write_ = nullptr;
      curl_write_callback header_ = nullptr;

      void* write_data_ = nullptr;
      void* header_data_ = nullptr;

      bool     local_ = false;
      bool     delivered_ = false;
      CURLcode outcome_ = CURLE_OK;

      response answer_;
    };
  }
}
