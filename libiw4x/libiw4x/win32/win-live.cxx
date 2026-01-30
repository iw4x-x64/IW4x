#include <libiw4x/win32/win-live.hxx>

namespace iw4x
{
  namespace win32
  {
    int64_t
    live_start_signin_any (unsigned int)
    {
      LOG_TRACE_L3 (ctx.log, __func__);

      g_waitForConnectionDialogState = 0;
      return true;
    }

    char
    live_base_game_license_check ()
    {
      LOG_TRACE_L3 (ctx.log, __func__);

      // @@: The original implementation goes through various GDK-related
      // checks. While doing so, it also calls Live_IsSignedIn(), which
      // suggests there may be additional state or ordering constraints we
      // will eventually need to mirror.
      //
      return true;
    }
  }
}
