#pragma once

#include <cstdint>
#include <cstddef>

#include <libiw4x/gdk/path.hxx>
#include <libiw4x/gdk/text.hxx>
#include <libiw4x/gdk/types.hxx>
#include <libiw4x/gdk/async.hxx>
#include <libiw4x/gdk/vtable.hxx>

namespace iw4x
{
  namespace gdk
  {
    inline constexpr std::size_t blob_name_capacity (64);
    inline constexpr std::size_t max_blob_writes (32);
    inline constexpr std::size_t max_blob_reads (32);
    inline constexpr std::size_t max_containers (16);
    inline constexpr std::size_t max_updates (16);

    struct blob_info
    {
      const char*   name;
      std::uint32_t size;
    };

    struct save_blob
    {
      blob_info     info;
      std::uint8_t* data;
    };

    static_assert (offsetof (save_blob, info.size) == 8,
                   "0x1401B21B8 reads the blob size at byte 8");
    static_assert (offsetof (save_blob, data) == 16,
                   "0x1401B21A1 reads the blob data pointer at byte 16");
    static_assert (sizeof (save_blob) == 24, "recovered XGameSaveBlob size");

    struct container_info
    {
      const char*   name;
      const char*   display_name;
      std::uint32_t blob_count;
      std::uint32_t reserved;
      std::uint64_t total_size;
      std::int64_t  last_modified;
      bool          needs_sync;
    };

    static_assert (offsetof (container_info, last_modified) == 32,
                   "0x1401B1F3C reads lastModifiedTime at byte 32");

    using container_info_callback = bool (*) (const container_info*, void*);

    struct save_provider
    {
      path root;
      int  open = 0;
    };

    struct save_container
    {
      path                       location;
      text<blob_name_capacity>   name;
    };

    struct pending_write
    {
      text<blob_name_capacity> name;
      blob                     data;
    };

    struct save_update
    {
      save_container* target = nullptr;
      pending_write   writes[max_blob_writes];
      std::size_t     count = 0;
    };

    bool
    storable (chars) noexcept;

    struct xgame_save
    {
      static constexpr char name[] = "XGameSave";

      static constexpr guid api
      {
        0x704C3F58, 0xE629, 0x4CC2,
        {0xB1, 0x97, 0x30, 0x51, 0x1B, 0x99, 0x6F, 0xE2}
      };

      static constexpr guid id
      {
        0x704C3F58, 0xE629, 0x4CC2,
        {0xB1, 0x97, 0x30, 0x51, 0x1B, 0x99, 0x6E, 0xE2}
      };

      static constexpr feature features[] {feature::game_save};

      [[= slot {0x20}]] static HRESULT WINAPI
      initialize_provider_async (void*,
                                 void* user,
                                 const char* scid,
                                 bool sync_on_demand,
                                 async_block*) noexcept;

      [[= slot {0x28}]] static HRESULT WINAPI
      initialize_provider_result (void*,
                                  async_block*,
                                  save_provider**) noexcept;

      [[= slot {0x30}]] static void WINAPI
      close_provider (void*, save_provider*) noexcept;

      [[= slot {0x70}]] static HRESULT WINAPI
      enumerate_container_info (void*,
                                save_provider*,
                                void* context,
                                container_info_callback) noexcept;

      [[= slot {0x80}]] static HRESULT WINAPI
      create_container (void*,
                        save_provider*,
                        const char* name,
                        save_container**) noexcept;

      [[= slot {0x88}]] static void WINAPI
      close_container (void*, save_container*) noexcept;

      [[= slot {0xA8}]] static HRESULT WINAPI
      read_blob_data_async (void*,
                            save_container*,
                            const char* const* names,
                            std::uint32_t count,
                            async_block*) noexcept;

      [[= slot {0xB0}]] static HRESULT WINAPI
      read_blob_data (void*,
                      async_block*,
                      std::size_t size,
                      void* buffer,
                      std::uint32_t* count) noexcept;

      [[= slot {0xB8}]] static HRESULT WINAPI
      create_update (void*,
                     save_container*,
                     const char* display_name,
                     save_update**) noexcept;

      [[= slot {0xC0}]] static void WINAPI
      close_update (void*, save_update*) noexcept;

      [[= slot {0xC8}]] static HRESULT WINAPI
      submit_blob_write (void*,
                         save_update*,
                         const char* name,
                         const std::uint8_t* data,
                         std::uint32_t size) noexcept;

      [[= slot {0xE0}]] static HRESULT WINAPI
      submit_update_async (void*, save_update*, async_block*) noexcept;

      [[= slot {0xE8}]] static HRESULT WINAPI
      submit_update (void*, async_block*) noexcept;
    };
  }
}
