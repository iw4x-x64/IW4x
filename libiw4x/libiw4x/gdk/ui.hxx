#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>

#include <libiw4x/gdk/types.hxx>
#include <libiw4x/gdk/async.hxx>
#include <libiw4x/gdk/vtable.hxx>

namespace iw4x
{
  namespace gdk
  {
    using player_picker = std::size_t (*) (std::string_view prompt,
                                           const std::uint64_t* candidates,
                                           std::size_t count,
                                           std::uint64_t* chosen,
                                           std::size_t maximum);

    void
    pick_players_with (player_picker) noexcept;

    inline constexpr std::size_t max_candidates (4096);

    struct xgame_ui
    {
      static constexpr char label[] = "XGameUi";

      static constexpr guid api
      {
        0xDFCD4649, 0x4FF8, 0x4043,
        {0xBA, 0x07, 0x35, 0xD6, 0x07, 0xDF, 0x98, 0xB0}
      };

      static constexpr guid id
      {
        0x36A03122, 0x9EA3, 0x4A3A,
        {0xA8, 0xA4, 0x89, 0x9C, 0xFD, 0x85, 0xD7, 0xDB}
      };

      static constexpr feature features[] {feature::game_ui};

      [[= slot {0x58}]] static HRESULT WINAPI
      show_player_picker_async (void*,
                                async_block*,
                                void* user,
                                const char* prompt,
                                std::uint32_t count,
                                const std::uint64_t* xuids,
                                std::uint32_t,
                                const void*,
                                std::uint32_t,
                                std::uint32_t maximum) noexcept;

      [[= slot {0x68}]] static HRESULT WINAPI
      show_player_picker_result (void*,
                                 async_block*,
                                 std::uint32_t maximum,
                                 std::uint64_t* out,
                                 std::uint32_t* count) noexcept;
    };
  }
}
