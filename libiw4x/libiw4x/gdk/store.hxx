#pragma once

#include <cstdint>
#include <cstddef>

#include <libiw4x/gdk/types.hxx>
#include <libiw4x/gdk/async.hxx>
#include <libiw4x/gdk/vtable.hxx>

namespace iw4x
{
  namespace gdk
  {
    struct game_license
    {
      char          sku_store_id[18];
      bool          active;
      bool          trial_owned_by_this_user;
      bool          disc_license;
      bool          trial;
      std::uint32_t expiration_date;
      std::uint32_t trial_time_remaining;
      char          trial_unique_id[64];
    };

    static_assert (offsetof (game_license, active) == 18,
                   "0x1401B4320 reads isActive at byte 18");
    static_assert (offsetof (game_license, trial) == 21,
                   "0x1401B4320 reads isTrial at byte 21");
    static_assert (sizeof (game_license) == 96,
                   "recovered XStoreGameLicense size");

    struct store_context
    {
      int open = 0;
    };

    struct xstore
    {
      static constexpr char name[] = "XStore";

      static constexpr guid api
      {
        0x0DD112AC, 0x7C24, 0x448C,
        {0xB9, 0x2B, 0x39, 0x60, 0xFB, 0x5B, 0xD3, 0x0C}
      };

      static constexpr guid id
      {
        0x5C48DEDF, 0x0B67, 0x4492,
        {0xA4, 0xB5, 0x68, 0x29, 0xB8, 0xE7, 0x96, 0xE1}
      };

      static constexpr feature features[] {feature::store};

      static constexpr std::size_t declined[]
      {
        0x48,
        0x50,
        0x78,
        0x88,
        0x98,
        0xA0,
        0xA8,
        0xB8,
        0xD0,
        0xD8
      };

      static constexpr std::size_t declined_false[] {0x80, 0xB0};

      [[= slot {0x18}]] static HRESULT WINAPI
      create_context (void*, void* user, store_context**) noexcept;

      [[= slot {0x20}]] static void WINAPI
      close_context_handle (void*, store_context*) noexcept;

      [[= slot {0xE0}]] static HRESULT WINAPI
      query_game_license_async (void*, store_context*, async_block*) noexcept;

      [[= slot {0xE8}]] static HRESULT WINAPI
      query_game_license_result (void*, async_block*, game_license*) noexcept;
    };
  }
}
