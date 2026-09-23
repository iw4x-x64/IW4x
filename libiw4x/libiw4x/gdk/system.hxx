#pragma once

#include <cstddef>

#include <libiw4x/gdk/types.hxx>
#include <libiw4x/gdk/vtable.hxx>

namespace iw4x
{
  namespace gdk
  {
    struct xsystem
    {
      static constexpr char label[] = "XSystem";

      static constexpr guid api
      {
        0xE349BD1A, 0xFC20, 0x4E40,
        {0xB9, 0x9C, 0x41, 0x78, 0xCC, 0x6B, 0x40, 0x9F}
      };

      static constexpr guid id
      {
        0xDADC2895, 0x34B0, 0x4EF5,
        {0xA8, 0x3E, 0x45, 0x11, 0x4D, 0x62, 0x9B, 0x80}
      };

      static constexpr feature features[] {feature::system, feature::error};

      static constexpr char sandbox[] = "RETAIL";

      [[= slot {0x20}]] static HRESULT WINAPI
      get_xbox_live_sandbox_id (void*,
                                std::size_t,
                                char*,
                                std::size_t*) noexcept;
    };
  }
}
