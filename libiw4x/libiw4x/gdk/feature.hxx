#pragma once

#include <cstdint>

#include <libiw4x/gdk/types.hxx>
#include <libiw4x/gdk/vtable.hxx>

namespace iw4x
{
  namespace gdk
  {
    struct xruntime_feature
    {
      static constexpr char label[] = "XGameRuntimeFeature";

      static constexpr guid api
      {
        0x8836FE87, 0xEDB9, 0x4FE3,
        {0x8D, 0xAD, 0x05, 0xF0, 0xD2, 0xCD, 0x5B, 0x40}
      };

      static constexpr guid id {api};

      [[= slot {0x18}]] static char WINAPI
      is_feature_available (void*, std::uint32_t) noexcept;
    };
  }
}
