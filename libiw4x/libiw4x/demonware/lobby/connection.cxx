#include <libiw4x/demonware/lobby/connection.hxx>

#include <cstring>

#include <libiw4x/demonware/lobby/remote-task-manager/remote-task-manager.hxx>

namespace iw4x
{
  namespace demonware
  {
    alignas (16) bd_lobby_connection lobby_connection;

    namespace
    {
      // IW4 performs virtual dispatch on the connection object when it is being
      // destroyed. We provide a dummy destructor to catch this, and we fill the
      // rest of the vtable with safe defaults so the engine doesn't crash if it
      // tries to call into unidentified slots.
      //
      static constexpr size_t lobby_connection_vtable_slots (32);
      void* lobby_connection_vtable[lobby_connection_vtable_slots];

      int64_t
      lobby_connection_stub (void*)
      {
        return 0;
      }

      int64_t
      lobby_connection_destructor (bd_lobby_connection*)
      {
        return 0;
      }
    }

    int32_t
    lobby_connection_start_task (void* connection,
                                 void** task_out,
                                 uint8_t service_id,
                                 uint8_t sub_func_id,
                                 void* payload,
                                 float timeout)
    {
      auto task (
        remote_task_manager::start_task (service_id, sub_func_id, payload));

      // See if the caller expects a task object back.
      //
      if (task_out != nullptr)
        *task_out = task;

      return 0; // BD_NO_ERROR.
    }

    void
    lobby_connection_constructor ()
    {
      auto stub (reinterpret_cast<void*> (&lobby_connection_stub));

      for (auto& e : lobby_connection_vtable)
        e = stub;

      lobby_connection_vtable[0] =
        reinterpret_cast<void*> (&lobby_connection_destructor);
      lobby_connection.vtable = lobby_connection_vtable;
      lobby_connection.refcount = 0x7FFFFFFF; // never reaches zero

      memset (lobby_connection.body, 0, sizeof (lobby_connection.body));
    }
  }
}


namespace iw4x
{
  namespace demonware
  {
    alignas (16) bd_lobby_connection xlobby_connection;

    namespace
    {
      // IW4 performs virtual dispatch on the connection object when it is being
      // destroyed. We provide a dummy destructor to catch this, and we fill the
      // rest of the vtable with safe defaults so the engine doesn't crash if it
      // tries to call into unidentified slots.
      //
      static constexpr size_t xlobby_connection_vtable_slots (32);
      void* xlobby_connection_vtable[lobby_connection_vtable_slots];

      int64_t
      xlobby_connection_stub (void*)
      {
        return 0;
      }

      int64_t
      xlobby_connection_destructor (bd_lobby_connection*)
      {
        return 0;
      }
    }


    void
    xlobby_connection_constructor ()
    {
      auto stub (reinterpret_cast<void*> (&lobby_connection_stub));

      for (auto& e : lobby_connection_vtable)
        e = stub;

      lobby_connection_vtable[0] =
        reinterpret_cast<void*> (&lobby_connection_destructor);
      lobby_connection.vtable = lobby_connection_vtable;
      lobby_connection.refcount = 0x7FFFFFFF; // never reaches zero

      memset (lobby_connection.body, 0, sizeof (lobby_connection.body));
    }
  }
}
