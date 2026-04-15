#include <libiw4x/mod/mod-demonware.hxx>
#include "libiw4x/logger.hxx"

#include <algorithm>
#include <cassert>
#include <cstdint>

#include <libiw4x/detour.hxx>

#include <libiw4x/demonware/bd-auth-service.hxx>
#include <libiw4x/demonware/bd-bandwidth-test-client.hxx>
#include <libiw4x/demonware/bd-lobby-service.hxx>
#include <libiw4x/demonware/bd-remote-task.hxx>
#include <libiw4x/demonware/bd-remote-task-manager.hxx>
#include <libiw4x/demonware/bd-storage.hxx>
#include <libiw4x/demonware/bd-platform-log.hxx>

using namespace std;
using namespace iw4x::demonware;

namespace iw4x
{
  namespace mod
  {
    namespace
    {
      // Engine globals mapped directly into the executable's memory space.
      // We rely on these reference casts to mutate internal state without
      // needing to reverse-engineer the entire game structure.
      //
      auto& uk_state                (*reinterpret_cast<int32_t*>  (0x141646308));
      auto& uk_disable              (*reinterpret_cast<bool*>     (0x1416462E9));
      auto& uk_session_signin       (*reinterpret_cast<uint8_t*>  (0x141A4DA7C));
      auto& uk_session_signin_state (*reinterpret_cast<int32_t*>  (0x141A4dA84));
      auto& uk_xprivileg_state      (*reinterpret_cast<uint8_t*>  (0x141A4A990 + 0xC0));
      auto& uk_xprivileg_cache      (*reinterpret_cast<uint8_t*>  (0x141A4A990 + 0xC0 + 2));
      auto& uk_retry_count          (*reinterpret_cast<int32_t*>  (0x1416462C4));
      auto& uk_timestamp            (*reinterpret_cast<int32_t*>  (0x1416462C8));

      auto& dw_lobby_service        (*reinterpret_cast<void**>    (0x1416462D0));
      auto& dw_socket_router        (*reinterpret_cast<void**>    (0x1416462D8));
      auto& dw_session              (*reinterpret_cast<void**>    (0x141a4a998));
      auto& dw_session_context      (*reinterpret_cast<uint64_t*> (0x141a4a9a0));

      auto& g_haveValidPlaylists    (*reinterpret_cast<uint8_t*>  (0x1467E86B0));
      auto& s_playlistOutOfDate     (*reinterpret_cast<uint8_t*>  (0x141C39F60));
      auto& uk_playlist_state       (*reinterpret_cast<int32_t*>  (0x140465EB8));

      auto& stats_retry_count       (*reinterpret_cast<int32_t*>  (0x141648060));

      // Stats-related globals.
      //
      auto& uk_player_data_init     (*reinterpret_cast<uint8_t*>  (0x141C37F20));
      auto& uk_stats_fail_count     (*reinterpret_cast<int32_t*>  (0x140719AF8));

      using player_data_set_int_t = void (*) (int, const char*, int);
      player_data_set_int_t player_data_set_int (
        reinterpret_cast<player_data_set_int_t> (0x1401FE290));

      void
      iwnet_frame (int controller)
      {
        // Force the state variable to 0x08. It is not entirely clear what this
        // state actually represents in the grand scheme of things, but this is
        // the exact value the game expects here. It might simply be another
        // peculiarity of the GDK we have to accommodate.
        //
        uk_state = 8;

        // Clear the disable flag. The game tends to set this flag to true
        // immediately prior to calling into the Demonware service, which
        // completely blocks the service from progressing. Like the state
        // variable above, the exact semantics escape us, but overriding it here
        // solves the lock-up.
        //
        uk_disable = false;

        // Force a session sign-in byte on every tick. While its exact mechanics
        // are still unconfirmed, testing shows that Live_Frame silently skips
        // some important logic if this isn't set.
        //
        uk_session_signin = 1;

        // Resolve Xbox Live sign-in state at 0x141A4dA84.
        //
        // The core issue here is that the connectivity gate at 0x1402A6F40
        // (check 2) calls into 0x1401B5AA0, which stubbornly expects this state
        // to be 3 (meaning signed in). Under normal conditions, the GDK sign-in
        // flow at 0x1401b62E0 handles this for us. But because we've stripped
        // the GDK initialization out entirely, that flow never runs. We are
        // forced to manually spoof the signed-in state right here to squeeze
        // past the gate.
        //
        uk_session_signin_state = 3;

        // Multiplayer privilege cache entry for controller 0.
        //
        // The third check in the connectivity gate at 0x1401b5E90 verifies our
        // multiplayer privileges by indexing into the session array located at
        // 0x141A4A990. The index logic is roughly (privilege_id + 3) * 12. So,
        // for privilege 13 (XPRIVILEGE_MULTIPLAYER_SESSIONS), we land perfectly
        // at offset 0xC0. Note that byte +2 acts as a "checked" flag, which we
        // also need to spoof.
        //
        uk_xprivileg_state = 1;
        uk_xprivileg_cache = 1;

        if (static bool connected (false); !connected)
        {
          connected = true;

          // Disable the automated background retry mechanism.
          //
          // Because we are now driving the Demonware pump manually within our
          // own frame loop, the original background retry system (and its
          // associated timestamp tracking) is completely redundant and often
          // conflicts with our state.
          //
          uk_retry_count = 0;
          uk_timestamp = 0;

          Uk_OnConnected (controller);

          // Populate the session name. This must happen AFTER
          // live_on_connected, otherwise the game immediately zeroes out the
          // session array and deletes our string.
          //
          auto sn (reinterpret_cast<char*> (0x141A4A998 + 0x30D4));
          strncpy (sn, auth_service::ticket ().username, 16 - 1);
          sn[16 - 1] = '\0';

          uk_session_signin = 1;
        }

        // We are done with the heavy lifting, so just pump the task completion array.
        //
        DW_SendPush ();
      }

      // Router definition.
      //
      struct bd_router
      {
        void* vtable;
        uint8_t body[0x1F8];
      };

      static_assert (sizeof (bd_router) == 0x200);

      alignas (16) bd_router router {};

      [[gnu::ms_abi]] int64_t
      router_stub (void*)
      {
        return 0;
      }

      static constexpr size_t router_vtable_slots (64);
      void* router_vtable[router_vtable_slots];

      void
      router_init ()
      {
        // Wire up our dummy vtable. We route all virtual calls to a single stub
        // to prevent crashes when the engine attempts to tear down or query the
        // socket router.
        //
        auto stub (reinterpret_cast<void*> (&router_stub));

        for (auto& e : router_vtable)
          e = stub;

        router.vtable = router_vtable;
      }

      using bdSocketRouterConnect_t =
        bool (__thiscall*) (void* self, void* addr_handle_ref);

      bdSocketRouterConnect_t bdSocketRouterConnect =
        reinterpret_cast<bdSocketRouterConnect_t> (0x140335340);

      // After a friend connects, DW_SendPush (called every frame) ends up
      // invoking bdSocketRouter::connect on the real bdSocketRouter embedded
      // inside bdNetImpl (+0x308).  That triggers NAT trav with an invalid
      // socket and bogus addresses (2.0.0.0:0), which loops "Failed to send.
      // Invalid socket?"
      //
      bool __thiscall
      socket_router_connect (void*, void*)
      {
        return false;
      }

      // eek at the first byte of every pending datagram before deciding
      //   whether to consume it.  STUN packets top two bits
      //   of the first byte are always 00) belong to bdNet and are passed
      //   through to the original receiveFrom so NAT traversal continues
      //   to work.  All other packets (OOB 0xFF… and Netchan game traffic)
      //   are left in the kernel socket buffer by returning -2 (bd_wouldblock)
      //   without consuming them.  NET_GetPacket then reads them directly
      //   via recvfrom(), processes them in the same frame, and feeds them
      //   to SV_PacketEvent / CL_PacketEvent with correct sequencing.
      //
      //
      using bdSocketReceiveFrom_t =
        int (__thiscall*) (void* self, void* out_addr,
                           void* out_buf, int buf_size);

      bdSocketReceiveFrom_t bdSocketReceiveFrom =
        reinterpret_cast<bdSocketReceiveFrom_t> (0x140371fc0);

      int __thiscall
      socket_receive_from (void* self, void* out_addr,
                                 void* out_buf, int buf_size)
      {
        // Socket fd is stored at bdSocket + 0x08.
        //
        auto fd (*reinterpret_cast<int32_t*> (
          static_cast<char*> (self) + 0x08));

        if (fd == -1)
          return -2;

        // Peek at the first byte without consuming the datagram.
        //
        unsigned char first_byte = 0;
        int peeked (recv (static_cast<SOCKET> (static_cast<uint32_t> (fd)),
                          reinterpret_cast<char*> (&first_byte),
                          1,
                          MSG_PEEK));

        if (peeked <= 0)
        {
          if (peeked == -1 && WSAGetLastError() == WSAEMSGSIZE)
            ;
          else
            return -2;
        }

        // STUN packets have their top two bits == 00 (RFC 5389). Pass
        // them to bdNet so the NAT traversal / IP discovery subsystem
        // keeps working.
        //
        if ((first_byte & 0xC0) == 0x00)
          return bdSocketReceiveFrom (self, out_addr,
                                                out_buf, buf_size);

        // Game packet (OOB 0xFF… or Netchan): leave it in the socket
        // buffer. NET_GetPacket will consume it via recvfrom() in the
        // same frame and feed it to the engine with correct sequencing.
        //
        return -2;
      }


      // Stub session object to satisfy the engine's pointer checks.
      //
      // The gate at 0x1401B5A50 absolutely insists that the session pointer is
      // non-null. Furthermore, 0x1401B5BE3 will blindly dereference [ptr +
      // 0x28]. That dereferenced pointer must then be either null (which causes
      // a graceful failure we can handle) or point to a valid refcounted
      // object.
      //
      alignas (16) uint8_t session_stub[0x30] {};

      using uk_is_session_clear_t = bool (*) (int);
      uk_is_session_clear_t is_session_clear (
        reinterpret_cast<uk_is_session_clear_t> (0x1401B5AC0));

      using uk_get_state_t = int (*) ();
      uk_get_state_t uk_get_state (
        reinterpret_cast<uk_get_state_t> (0x14012F720));

      int32_t last_playlist_state (-1);

      void __attribute__ ((ms_abi))
      live_storage_fetch_playlists (int controller)
      {
        if (uk_playlist_state != last_playlist_state)
        {
          last_playlist_state = uk_playlist_state;

          // We intentionally evaluate these purely for their side effects. The
          // engine requires the internal state modifications that happen inside
          // these getters before we actually trigger the playlist fetch.
          //
          auto session (is_session_clear (controller));
          auto state (uk_get_state ());

          (void)session;
          (void)state;
        }

        LiveStorage_FetchPlaylists (controller);
      }

      bool stats_diag_logged (false);

      using has_active_slot_t = bool (*) (void*, int, int);
      has_active_slot_t has_active_slot (
        reinterpret_cast<has_active_slot_t> (0x1402005C0));

      using readiness_check_t = bool (*) (int);
      readiness_check_t readiness_check (
        reinterpret_cast<readiness_check_t> (0x1401FE180));

      using get_status_accessor_t = bool (*) (int);
      get_status_accessor_t get_status_accessor (
        reinterpret_cast<get_status_accessor_t> (0x1401358E0));

      int32_t uk_stats_last_game_state (-1);
      uint8_t uk_stats_last_playlists (0xFF);

      void __attribute__ ((ms_abi))
      live_storage_download_stats_from_dir (int controller)
      {
        if (uk_playlist_state != uk_stats_last_game_state ||
            g_haveValidPlaylists != uk_stats_last_playlists)
        {
          // Similar to the playlist fetch above, these calls are executed to
          // tick internal engine states related to profile slot activity and
          // general readiness.
          //
          auto g1 (has_active_slot (reinterpret_cast<void*> (0x141C39FB0), 1, controller));
          auto g2 (readiness_check (controller));
          auto g3 (is_session_clear (controller));
          auto g4 (get_status_accessor (controller));

          (void)g1;
          (void)g2;
          (void)g3;
          (void)g4;
        }

        LiveStorage_DownloadStatsFromDir (controller);
      }

      bool profile_init_diag_logged (false);

      void* __attribute__ ((ms_abi))
      client_connect (int controller, uint16_t mode)
      {
        if (!profile_init_diag_logged)
          profile_init_diag_logged = true;

        return ClientConnect (controller, mode);
      }

      using any_session_signed_in_t = bool (*) ();
      any_session_signed_in_t any_session_signed_in (
        reinterpret_cast<any_session_signed_in_t> (0x1401b5a70));

      bool signin_diag_logged (false);

      void __attribute__ ((ms_abi))
      live_frame (int controller)
      {
        // Poke the session gate to keep the internal sign-in watchdog happy.
        //
        auto gate (any_session_signed_in ());
        (void)gate;

        if (!signin_diag_logged)
          signin_diag_logged = true;

        Live_Frame (controller);
      }
    }

    demonware_module::
    demonware_module ()
    {
      // Initialize our custom Demonware service implementations.
      //
      demonware::auth_service ();
      demonware::lobby_service ();
      demonware::remote_task_manager ();
      demonware::storage ();
      demonware::bandwidth_test_client ();

      router_init ();

      dw_socket_router = &router;
      dw_lobby_service = &lobby_service::instance ();

      uk_state = 8;
      uk_disable = false;

      auto& session_ptr (*reinterpret_cast<void**> (0x141a4a998));
      auto& session_context (*reinterpret_cast<uint64_t*> (0x141a4a9a0));

      session_ptr = session_stub;
      session_context = auth_service::ticket ().user_id;

      detour (bdLogMessage, &bd_log_message);
      detour (IWNet_Frame,  &iwnet_frame);
      detour (LiveStorage_FetchPlaylists, &live_storage_fetch_playlists);
      detour (LiveStorage_DownloadStatsFromDir, &live_storage_download_stats_from_dir);
      detour (ClientConnect, &client_connect);
      detour (Live_Frame, live_frame);
      detour (bdSocketRouterConnect, socket_router_connect);
      detour (bdSocketReceiveFrom, socket_receive_from);
    }
  }
}
