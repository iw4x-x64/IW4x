#include <libiw4x/mod/mod-live.hxx>

#include <string_view>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>

#include <libnp/np.hxx>

#include <libiw4x/context.hxx>
#include <libiw4x/detour.hxx>
#include <libiw4x/logger.hxx>
#include <libiw4x/scheduler.hxx>

using namespace iw4x::log;
using C = categories::iw4x;

namespace iw4x
{
  namespace mod
  {
    namespace
    {
      bool& have_valid_playlists (*reinterpret_cast<bool*> (0x1467E86B0));
      bool& playlist_out_of_date (*reinterpret_cast<bool*> (0x141C39F60));

      void
      live_storage_fetch_playlists (int ci)
      {
        scheduler::spawn ([] () -> boost::asio::awaitable<void>
        {
          try
          {
            np::playlist_info pi (co_await ctx.np.storage ().get_playlist ());

            // IW manages a static memory region for the playlist payload. We
            // simply override the pointers to use its static buffer directly.
            //
            uint8_t*  p  (reinterpret_cast<uint8_t*>   (0x1467E86C0));
            uint32_t& ps (*reinterpret_cast<uint32_t*> (0x1468086E4));
            uint8_t*& pp (*reinterpret_cast<uint8_t**> (0x1468086E8));

            ps = 0x20000;
            pp = p;

            if (!pi.buffer.empty () && pi.buffer.size () < ps)
            {
              // Note that the internal parser uses standard C-string functions
              // internally, so we the buffer must be null-terminated.
              //
              memcpy (p, pi.buffer.data (), pi.buffer.size ());
              p[pi.buffer.size ()] = '\0';

              Playlist_ParsePlaylists (p);

              have_valid_playlists = true;
              playlist_out_of_date = false;

              // Clear the engine's internal error state. If a previous fetch
              // failed (e.g., during startup when the network interface wasn't
              // ready), the engine might have latched a non-zero error code
              // here. We reset it to 0 so the UI doesn't display a ghost error.
              //
              uint32_t& err (*reinterpret_cast<uint32_t*> (0x1468086C0));
              err = 0;
            }
          }
          catch (const std::exception& e)
          {
            error {C {}, "unable to fetch playlists: {}", e.what ()};
          }

          co_return;
        });
      }

      bool
      live_storage_is_waiting_on_playlists ()
      {
        // The original implementation first queried the Live Storage task
        // system to determine whether the playlist fetch task was still active.
        // Only if the task had completed would it inspect the playlist state
        // itself.
        //
        // Here we drop the task system and move the fetch logic to be driven
        // directly by our scheduler. That is, we derive the waiting state
        // purely from the validity of the playlist data: if we either do not
        // yet have valid playlists or the current playlists are known to be
        // stale, the system is considered to still be waiting for an update.
        //
        return !have_valid_playlists || playlist_out_of_date;
      }

      bool
      live_storage_is_waiting_on_stats (int c)
      {
        int n (CL_LocalClientNumFromControllerIndex ());

        // Map the static base address of the game's stats pool, and create a
        // dummy stats source (level 1). The per-client stats block size appears
        // to be 0x4018 bytes.
        //
        uint8_t* stats_source (reinterpret_cast<uint8_t*> (0x141C37F20));
        uint8_t& b (stats_source[n * 0x4018]);

        if (b == 0)
          b = 1;

        // We are never waiting on the network as we just provided the dummy
        // data ourselves. Tell the caller to proceed.
        //
        return false;
      }

      void
      live_throw_error (int code, const char* error_key)
      {
        std::string_view const e (error_key);

        if (e != "MP_BUILDEXPIRED" && e != "XBOXLIVE_SIGNEDOUT")
          return;

        if (!Com_ErrorEntered ())
          Com_Error (code, error_key);
      }
    }

    live_module::
    live_module ()
    {
      detour (LiveStorage_IsWaitingOnPlaylists, &live_storage_is_waiting_on_playlists);
      detour (LiveStorage_FetchPlaylists, &live_storage_fetch_playlists);
      detour (LiveStorage_IsWaitingOnStats, &live_storage_is_waiting_on_stats);
      detour (Live_ThrowError, &live_throw_error);
    }
  }
}
