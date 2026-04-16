#include <libiw4x/demonware/lobby/lobby-service.hxx>

#include <cassert>
#include <cstdint>
#include <cstring>

#include <libiw4x/import.hxx>
#include <libiw4x/detour.hxx>
#include <libiw4x/logger.hxx>

#include <libiw4x/demonware/lobby/storage/storage.hxx>

using namespace std;

namespace iw4x
{
  namespace demonware
  {
    namespace
    {
      alignas (16) bd_lobby_service_impl lobby_service_impl;

      // Note that these slots map exactly to the original game's vtable layout
      // at 0x1403DBAC0. We only have four slots to worry about here:
      //
      //   [0] destructor   (0x14031C4F0)
      //   [1] onConnect    (0x14031D090)
      //   [2] pump         (0x140326620)
      //   [3] onDisconnect (0x14031D320)
      //
      void
      lobby_service_impl_destructor (bd_lobby_service_impl*)
      {
        // Since our singleton is statically allocated and lives for the
        // duration of the process, there is nothing to actually free here. We
        // just leave it empty.
        //
      }

      void
      lobby_service_impl_on_connect (bd_lobby_service_impl*)
      {
        // The connection is inherently established in our local emulation
        // context, so we can just no-op this step without causing any issues
        // downstream.
        //
      }

      void
      lobby_service_impl_pump (bd_lobby_service_impl*)
      {
        // We handle task completion inside startTask directly, so the pump
        // mechanism does not need to do anything.
        //
      }

      void
      lobby_service_impl_on_disconnect (bd_lobby_service_impl*)
      {
        // We never actually disconnect from the emulated lobby service.
        //
      }

      void* lobby_service_impl_vtable[4]
      {
        reinterpret_cast<void*> (&lobby_service_impl_destructor),
        reinterpret_cast<void*> (&lobby_service_impl_on_connect),
        reinterpret_cast<void*> (&lobby_service_impl_pump),
        reinterpret_cast<void*> (&lobby_service_impl_on_disconnect)
      };

      void
      lobby_service_impl_constructor ()
      {
        memset (&lobby_service_impl, 0, sizeof (lobby_service_impl));

        lobby_service_impl.vtable = lobby_service_impl_vtable;
        lobby_service_impl.connection = &lobby_connection;

        bd_storage.status = 0;
        bd_storage.connection = &lobby_connection;

        lobby_service_impl.storage = &bd_storage;
      }

      // Non-virtual member function hooks.
      //

      bool
      lobby_service_impl_connect (void*, void*, void*, bool)
      {
        // We pretend the connection succeeds immediately and is always available.
        //
        return true;
      }

      void
      lobby_service_impl_disconnect (void*, int /* reason */)
      {
        // We do not have anything to tear down, so this is just a stub.
        //
      }

      int32_t
      lobby_service_impl_get_status (void*)
      {
        // Always return BD_ONLINE, which corresponds to the value 2.
        //
        return 2;
      }

      // The following are straightforward getters for the sub-services. Since
      // the instance is globally maintained by us, we simply return the
      // respective pointers from our singleton.
      //

      void*
      lobby_service_impl_get_matchmaking (void*)
      {
        return lobby_service_impl.matchmaking;
      }

      void*
      lobby_service_impl_get_task_mgr (void*)
      {
        return lobby_service_impl.task_mgr;
      }

      void*
      lobby_service_impl_get_performance (void*)
      {
        return lobby_service_impl.performance;
      }

      void*
      lobby_service_impl_get_storage (void*)
      {
        return lobby_service_impl.storage;
      }

      void*
      lobby_service_impl_instance ()
      {
        return &lobby_service_impl;
      }
    }

    lobby_service::
    lobby_service ()
    {
      lobby_connection_constructor ();
      lobby_service_impl_constructor ();

      detour (bdLobbyService,                    &lobby_service_impl_instance);
      detour (bdLobbyServiceImplGetMatchmaking,  &lobby_service_impl_get_matchmaking);
      detour (bdLobbyServiceImplGetPerformance,  &lobby_service_impl_get_performance);
      detour (bdLobbyServiceImplGetStatus,       &lobby_service_impl_get_status);
      detour (bdLobbyServiceImplGetStorage,      &lobby_service_impl_get_storage);
      detour (bdLobbyServiceImplGetTaskMgr,      &lobby_service_impl_get_task_mgr);
      detour (bdLobbyServiceImplConnect,         &lobby_service_impl_connect);
      detour (bdLobbyServiceImplDisconnect,      &lobby_service_impl_disconnect);
    }

    bd_lobby_service_impl&
    lobby_service::impl ()
    {
      return lobby_service_impl;
    }
  }
}
