#pragma once

#include <cstdint>

namespace iw4x
{
  namespace demonware
  {
    // bdLobbyConnection compatible object layout.
    //
    struct bd_lobby_connection
    {
      void*   vtable;      // 0x00
      int32_t refcount;    // 0x08
      uint8_t body [0xF4]; // 0x0C: zero-initialized
    };

    static_assert (sizeof (bd_lobby_connection) == 0x100);

    void
    lobby_connection_constructor ();

    int32_t
    lobby_connection_start_task (void*   connection,
                                 void**  task_out,
                                 uint8_t service_id,
                                 uint8_t sub_func_id,
                                 void*   payload,
                                 float   timeout);

    extern bd_lobby_connection lobby_connection;
  }
}
