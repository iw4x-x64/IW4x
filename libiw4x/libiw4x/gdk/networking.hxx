#pragma once

#include <cstdint>
#include <cstddef>

#include <libiw4x/gdk/types.hxx>
#include <libiw4x/gdk/vtable.hxx>
#include <libiw4x/gdk/async.hxx>

namespace iw4x
{
  namespace gdk
  {
    struct connectivity_hint
    {
      std::uint32_t level;
      std::uint32_t iana_interface_type;
      std::uint32_t cost;
      bool          network_initialized;
      bool          approaching_data_limit;
      bool          over_data_limit;
      bool          roaming;
    };

    static_assert (sizeof (connectivity_hint) == 16,
                   "recovered XNetworkingConnectivityHint size");
    static_assert (offsetof (connectivity_hint, network_initialized) == 12,
                   "libHttpClient reads networkInitialized at byte 12");

    enum class connectivity_level: std::uint32_t
    {
      unknown                     = 0,
      none                        = 1,
      local_access                = 2,
      internet_access             = 3,
      constrained_internet_access = 4,
      hidden                      = 5
    };

    enum class connectivity_cost: std::uint32_t
    {
      unknown      = 0,
      unrestricted = 1,
      fixed        = 2,
      variable     = 3
    };

    struct xnetworking
    {
      static constexpr char name[] = "XNetworking";

      static constexpr guid api
      {
        0x37E56907, 0x2F10, 0x41E8,
        {0xB7, 0x2F, 0x36, 0xED, 0xB1, 0x85, 0x33, 0x1A}
      };

      static constexpr guid id
      {
        0xBF2346B2, 0x39AF, 0x4658,
        {0xB5, 0xEA, 0x44, 0x71, 0x3C, 0x7E, 0x83, 0xB3}
      };

      static constexpr feature features[] {feature::networking};

      [[= slot {0x40}]] [[= slot {0x58}]] static HRESULT WINAPI
      query_security_information_async (void*,
                                        const void*,
                                        async_block*) noexcept;

      [[= slot {0x48}]] [[= slot {0x60}]] static HRESULT WINAPI
      query_security_information_result_size (void*,
                                              async_block*,
                                              std::size_t*) noexcept;

      [[= slot {0x50}]] [[= slot {0x68}]] static HRESULT WINAPI
      query_security_information_result (void*,
                                         async_block*,
                                         std::size_t size,
                                         std::size_t* used,
                                         void*,
                                         void** out) noexcept;

      [[= slot {0x70}]] static HRESULT WINAPI
      verify_server_certificate (void*, void*, const void*) noexcept;

      [[= slot {0x78}]] static HRESULT WINAPI
      get_connectivity_hint (void*, connectivity_hint*) noexcept;

      [[= slot {0x80}]] static HRESULT WINAPI
      register_connectivity_changed (
        void*,
        void*,
        void* context,
        void (*callback) (void*, const connectivity_hint*),
        std::uint64_t* token) noexcept;

      [[= slot {0x88}]] static HRESULT WINAPI
      unregister_connectivity_changed (void*,
                                       std::uint64_t token,
                                       bool) noexcept;
    };
  }
}
