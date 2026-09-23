#pragma once

#include <atomic>
#include <cstdint>

#include <libiw4x/gdk/sync.hxx>

#include <libiw4x/xcurl/xcurl.hxx>
#include <libiw4x/xcurl/transfer.hxx>

namespace iw4x
{
  namespace xcurl
  {
    inline constexpr std::uint64_t multi_signature (0x495734582D4D4C54ULL);

    class multi
    {
    public:
      static constexpr std::uint32_t capacity = 32;

      multi () noexcept = default;

      bool
      ours () const noexcept
      {
        return signature_ == multi_signature;
      }

      void
      disown () noexcept
      {
        signature_ = 0;
      }

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

      [[gnu::noinline]] int
      answered () noexcept;

      std::uint64_t signature_ = 0;

      CURLM* handle_ = nullptr;

      mutable gdk::mutex mutex_;

      transfer*     local_[capacity] {};
      std::uint32_t locals_ = 0;

      std::atomic<std::uint32_t> answered_ {0};

      CURLMsg       messages_[capacity] {};
      std::uint32_t head_ = 0;
      std::uint32_t count_ = 0;

      CURLMsg current_ {};
    };
  }
}
