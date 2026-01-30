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
  }
}
