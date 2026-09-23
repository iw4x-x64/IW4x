#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>

#include <libiw4x/gdk/text.hxx>

namespace iw4x
{
  namespace gdk
  {
    inline constexpr std::size_t gamertag_capacity (101);

    enum class gamertag_component: std::uint32_t
    {
      classic       = 0,
      modern        = 1,
      modern_suffix = 2,
      unique_modern = 3
    };

    std::uint64_t
    xuid ();

    inline text<19>
    xuid_text (std::uint64_t v) noexcept
    {
      return text<19> ("{}", hex_number (v, 16));
    }

    std::size_t
    gamertag (char* buffer, std::size_t size) noexcept;

    void
    set_gamertag (std::string_view) noexcept;

    bool
    component_bound (gamertag_component, std::size_t&) noexcept;
  }
}
