#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <libiw4x/import.hxx>

namespace iw4x
{
  namespace mod
  {
    struct dvar_handle
    {
      dvar_t* ptr;
      dvarType expected_type;
    };

    bool
    dvar_handle_valid (dvar_handle h);

    struct dvar_decl
    {
      const char* name;
      dvarType type;
      DvarValue default_value;
      DvarLimits domain;
      DvarFlags flags;
      const char* description;
    };

    struct dvar_snapshot
    {
      std::string name;
      dvarType type;
      DvarFlags flags;
      bool modified;
      DvarValue current;
      DvarValue latched;
      DvarValue reset;
      DvarLimits domain;
      std::string current_text;
      std::string latched_text;
      std::string reset_text;
      std::string description;
      std::vector<std::string> enum_values;
    };

    struct dvar_create_info
    {
      const char* name;
      dvarType type;
      DvarFlags flags;
      DvarValue value;
      DvarLimits domain;
      const char* description;
      const char** enum_values;
      int enum_value_count;
    };

    void
    init_dvars ();

    void
    register_dvars ();

    void
    resolve_dvars ();

    void
    apply_dvar_descriptions ();

    dvar_handle
    require_engine_dvar (const char* name, dvarType expected_type);

    dvar_handle
    find_engine_dvar (const char* name, dvarType expected_type);

    bool
    get_dvar_bool (dvar_handle h);

    int
    get_dvar_int (dvar_handle h);

    float
    get_dvar_float (dvar_handle h);

    const char*
    get_dvar_string (dvar_handle h);

    std::int64_t
    get_dvar_int64 (dvar_handle h);

    std::uint64_t
    get_dvar_uint64 (dvar_handle h);

    void
    set_dvar_bool (dvar_handle h,
                   bool value,
                   DvarSetSource src = DVAR_SOURCE_INTERNAL);

    void
    set_dvar_int (dvar_handle h,
                  int value,
                  DvarSetSource src = DVAR_SOURCE_INTERNAL);

    void
    set_dvar_float (dvar_handle h,
                    float value,
                    DvarSetSource src = DVAR_SOURCE_INTERNAL);

    void
    set_dvar_string (dvar_handle h,
                     const char* value,
                     DvarSetSource src = DVAR_SOURCE_INTERNAL);

    void
    reset_dvar (dvar_handle h, DvarSetSource src = DVAR_SOURCE_INTERNAL);

    const char*
    dvar_description (const dvar_t* dvar);

    using for_each_callback = void (__fastcall*) (const dvar_t*, void*);

    void
    for_each_dvar (for_each_callback callback, void* user_data);

    std::vector<dvar_snapshot>
    snapshot_dvars ();

    dvar_t*
    create_dvar (const dvar_create_info& info);

    bool
    set_dvar_value (const char* name,
                    const DvarValue& value,
                    DvarSetSource src = DVAR_SOURCE_INTERNAL);

    bool
    set_dvar_reset_value (const char* name, const DvarValue& value);

    bool
    set_dvar_flags (const char* name, DvarFlags flags);

    bool
    set_dvar_domain (const char* name, const DvarLimits& domain);

    bool
    set_dvar_description (const char* name, const char* description);

    bool
    unregister_dvar (const char* name);

    bool
    value_in_domain (dvarType type,
                     const DvarValue& value,
                     const DvarLimits& domain);

    bool
    values_equal (dvarType type, const DvarValue& lhs, const DvarValue& rhs);

    dvar_t*
    register_int64 (const char* name,
                    std::int64_t value,
                    std::int64_t min,
                    std::int64_t max,
                    DvarFlags flags,
                    const char* description);

    dvar_t*
    register_uint64 (const char* name,
                     std::uint64_t value,
                     std::uint64_t min,
                     std::uint64_t max,
                     DvarFlags flags,
                     const char* description);

    void
    add_dvar_flags (dvar_t* dvar, DvarFlags flags);

    const char*
    dvar_value_to_string (const dvar_t* dvar, const DvarValue& value);

    inline constexpr std::size_t dvar_pool_capacity = 0x1000;
    inline constexpr std::size_t dvar_hash_buckets = 0x400;

    class dvar_module
    {
    public:
      dvar_module ();
    };
  }
}
