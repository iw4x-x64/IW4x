#pragma once

#include <cstdint>

#include <libiw4x/xcurl/xcurl.hxx>
#include <libiw4x/xcurl/transfer.hxx>

namespace iw4x
{
  namespace xcurl
  {
    class multi
    {
    public:
      static constexpr std::uint32_t capacity = 32;

      multi () noexcept = default;

      multi (const multi&) = delete;
      multi& operator= (const multi&) = delete;

      bool
      open () noexcept;

      void
      close () noexcept;

      CURLM*
      handle () const noexcept
      {
        return handle_;
      }

      CURLMcode
      add (transfer&) noexcept;

      CURLMcode
      remove (transfer&) noexcept;

      CURLMcode
      perform (int* running) noexcept;

      CURLMsg*
      read (int* left) noexcept;

      CURLMcode
      poll (curl_waitfd*, unsigned int, int timeout, int* ready) noexcept;

    private:
      bool
      queue (transfer&) noexcept;

      CURLM* handle_ = nullptr;

      mutable mutex mutex_;

      transfer*     local_[capacity] {};
      std::uint32_t locals_ = 0;

      CURLMsg       messages_[capacity] {};
      std::uint32_t head_ = 0;
      std::uint32_t count_ = 0;

      CURLMsg current_ {};
    };
  }
}
