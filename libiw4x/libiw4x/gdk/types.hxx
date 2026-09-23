#pragma once

#include <bit>
#include <array>
#include <cstdint>
#include <cstddef>

#include <windows.h>

#include <libiw4x/iw4x.hxx>

namespace iw4x
{
  namespace gdk
  {
    inline HMODULE
    image () noexcept
    {
      return reinterpret_cast<HMODULE> (&__ImageBase);
    }

    struct guid
    {
      std::uint32_t data1;
      std::uint16_t data2;
      std::uint16_t data3;
      std::uint8_t  data4[8];
    };

    static_assert (sizeof (guid) == 16, "recovered GUID size");
    static_assert (alignof (guid) == 4, "recovered GUID alignment");

    constexpr bool
    operator== (const guid& x, const guid& y) noexcept
    {
      using words = std::array<std::uint64_t, 2>;

      words a (std::bit_cast<words> (x));
      words b (std::bit_cast<words> (y));

      return ((a[0] ^ b[0]) | (a[1] ^ b[1])) == 0;
    }

    bool operator<  (const guid&, const guid&) = delete;
    bool operator>  (const guid&, const guid&) = delete;
    bool operator<= (const guid&, const guid&) = delete;
    bool operator>= (const guid&, const guid&) = delete;

    enum class feature: std::uint32_t
    {
      accessibility            = 0,
      app_capture              = 1,
      async                    = 2,
      async_provider           = 3,
      display                  = 4,
      game                     = 5,
      game_invite              = 6,
      game_save                = 7,
      game_ui                  = 8,
      launcher                 = 9,
      networking               = 10,
      package                  = 11,
      persistent_local_storage = 12,
      speech_synthesizer       = 13,
      store                    = 14,
      system                   = 15,
      task_queue               = 16,
      thread                   = 17,
      user                     = 18,
      error                    = 19,
      game_event               = 20,
      game_streaming           = 21
    };

    const char*
    name (feature) noexcept;

    inline constexpr HRESULT pending (static_cast<HRESULT> (0x8000000AL));
    inline constexpr HRESULT insufficient_buffer (HRESULT_FROM_WIN32 (ERROR_INSUFFICIENT_BUFFER));
  }
}
