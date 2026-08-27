#pragma once

#include <cstdint>
#include <cstddef>

#include <libiw4x/gdk/text.hxx>
#include <libiw4x/gdk/types.hxx>
#include <libiw4x/gdk/async.hxx>
#include <libiw4x/gdk/vtable.hxx>
#include <libiw4x/gdk/identity.hxx>

namespace iw4x
{
  namespace gdk
  {
    using service_predicate = bool (*) (chars method, chars url);

    inline constexpr std::size_t max_claimants (4);

    void
    serve (service_predicate) noexcept;

    unsigned
    claimant_count () noexcept;

    enum class user_state: std::uint32_t
    {
      signed_in   = 0,
      signing_out = 1,
      signed_out  = 2
    };

    enum class age_group: std::uint32_t
    {
      unknown = 0,
      child   = 1,
      teen    = 2,
      adult   = 3
    };

    enum class deny_reason: std::uint32_t
    {
      none              = 0,
      purchase_required = 1,
      restricted        = 2,
      banned            = 3,
      unknown           = 4
    };

    struct token_and_signature
    {
      std::size_t token_size;
      std::size_t signature_size;
      const char* token;
      const char* signature;
    };

    static_assert (sizeof (token_and_signature) == 32,
                   "recovered XUserGetTokenAndSignatureData size");
    static_assert (offsetof (token_and_signature, token) == 16,
                   "0x140326000 reads the token pointer at offset 16");

    struct xuser
    {
      static constexpr char name[] = "XUser";

      static constexpr guid api
      {
        0x01ACD177, 0x91F9, 0x4763,
        {0xA3, 0x8E, 0xCC, 0xBB, 0x55, 0xCE, 0x32, 0xE0}
      };

      static constexpr guid id
      {
        0x079415E3, 0x6727, 0x437F,
        {0x8E, 0x9D, 0x8F, 0x8F, 0x9B, 0x24, 0x39, 0xF7}
      };

      static constexpr feature features[] {feature::user};

      [[= slot {0x18}]] static HRESULT WINAPI
      duplicate_handle (void*, void*, void**) noexcept;

      [[= slot {0x20}]] static HRESULT WINAPI
      close_handle (void*, void*) noexcept;

      [[= slot {0x28}]] static std::int32_t WINAPI
      compare (void*, void*, void*) noexcept;

      [[= slot {0x38}]] static HRESULT WINAPI
      add_async (void*, std::uint32_t options, async_block*) noexcept;

      [[= slot {0x40}]] static HRESULT WINAPI
      add_result (void*, async_block*, void**) noexcept;

      [[= slot {0x48}]] static HRESULT WINAPI
      get_local_id (void*, void*, std::uint64_t*) noexcept;

      [[= slot {0x58}]] static HRESULT WINAPI
      get_id (void*, void*, std::uint64_t*) noexcept;

      [[= slot {0x68}]] static HRESULT WINAPI
      get_is_guest (void*, void*, bool*) noexcept;

      [[= slot {0x70}]] static HRESULT WINAPI
      get_state (void*, void*, user_state*) noexcept;

      [[= slot {0x98}]] static HRESULT WINAPI
      get_age_group (void*, void*, age_group*) noexcept;

      [[= slot {0xA0}]] static HRESULT WINAPI
      check_privilege (void*,
                       void*,
                       std::uint32_t options,
                       std::uint32_t privilege,
                       bool* result,
                       deny_reason* reason) noexcept;

      [[= slot {0xA8}]] static HRESULT WINAPI
      resolve_privilege_async (void*,
                               void*,
                               std::uint32_t options,
                               std::uint32_t privilege,
                               async_block*) noexcept;

      [[= slot {0xB0}]] static HRESULT WINAPI
      resolve_privilege_result (void*, async_block*) noexcept;

      [[= slot {0xB8}]] static HRESULT WINAPI
      get_token_and_signature_async (void*,
                                     void*,
                                     std::uint32_t options,
                                     const char* method,
                                     const char* url,
                                     std::size_t,
                                     const void*,
                                     std::size_t,
                                     const void*,
                                     async_block*) noexcept;

      [[= slot {0xC0}]] static HRESULT WINAPI
      get_token_and_signature_result_size (void*,
                                           async_block*,
                                           std::size_t*) noexcept;

      [[= slot {0xC8}]] static HRESULT WINAPI
      get_token_and_signature_result (void*,
                                      async_block*,
                                      std::size_t size,
                                      void* buffer,
                                      void** out,
                                      std::size_t* used) noexcept;

      [[= slot {0x108}]] static HRESULT WINAPI
      register_for_change (void*,
                           void*,
                           void* context,
                           void (*callback) (void*,
                                             std::uint64_t,
                                             std::uint32_t),
                           std::uint64_t* token) noexcept;

      [[= slot {0x110}]] static HRESULT WINAPI
      unregister_for_change (void*, std::uint64_t token, bool) noexcept;
    };

    struct xuser_gamertag
    {
      static constexpr char name[] = "XUser (gamertag)";

      static constexpr guid api {xuser::api};

      static constexpr guid id
      {
        0xCEF4FAC0, 0x7676, 0x4A94,
        {0xA1, 0x19, 0x4C, 0x43, 0xF9, 0xEB, 0x5B, 0x74}
      };

      [[= slot {0x18}]] static HRESULT WINAPI
      get_gamertag (void*,
                    void*,
                    gamertag_component,
                    std::size_t size,
                    char* buffer,
                    std::size_t* used) noexcept;
    };
  }
}
