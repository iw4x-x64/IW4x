#pragma once

#include <cstdint>

#include <libiw4x/gdk/text.hxx>
#include <libiw4x/gdk/types.hxx>
#include <libiw4x/gdk/vtable.hxx>

namespace iw4x
{
  namespace gdk
  {
    inline constexpr std::size_t activation_capacity (768);

    using activation = text<activation_capacity>;

    activation
    activation_uri (chars connection) noexcept;

    void
    deliver_invite (chars connection) noexcept;

    struct xgame_invite
    {
      static constexpr char name[] = "XGameInvite";

      static constexpr guid api
      {
        0x0651AAE2, 0x4012, 0x4077,
        {0xBF, 0x84, 0x8B, 0x90, 0x97, 0x09, 0x0E, 0x2C}
      };

      static constexpr guid id {api};

      static constexpr feature features[] {feature::game_invite};

      [[= slot {0x18}]] static HRESULT WINAPI
      register_for_event (void*,
                          void*,
                          void* context,
                          void (*callback) (void*, const char*),
                          std::uint64_t* token) noexcept;

      [[= slot {0x20}]] static char WINAPI
      unregister_for_event (void*, std::uint64_t token, bool) noexcept;
    };
  }
}
