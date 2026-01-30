#include <libiw4x/win32/init.hxx>

#include <libiw4x/win32/win-live.hxx>

namespace iw4x
{
  namespace win32
  {
    bool
    init ()
    {
      LOG_TRACE_L3(ctx.log, __func__);

      detour (Live_StartSigninAny, &live_start_signin_any);

      return true;
    }
  }
}
