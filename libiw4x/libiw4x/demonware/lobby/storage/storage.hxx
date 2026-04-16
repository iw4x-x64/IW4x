#pragma once

#include <cstdint>

namespace iw4x
{
  namespace demonware
  {
    // bdStorage compatible object layout.
    //
    struct bd_storage_stub
    {
      int32_t status;     // 0x00
      int32_t pad;        // 0x04
      void*   connection; // 0x08
    };

    static_assert (sizeof (bd_storage_stub) == 0x10);

    extern bd_storage_stub bd_storage;

    class storage
    {
    public:
      storage ();
    };
  }
}
