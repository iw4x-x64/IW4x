#include <libiw4x/windows/console-api.hxx>
#include <libiw4x/windows/process-threads-api.hxx>

namespace iw4x
{
  namespace windows
  {
    // Windows console events (Ctrl+C, Ctrl+Break, Close) typically trigger a
    // registered handler routine. If no handler intercepts the event, the
    // default behavior is to re-enters the dangerous CRT shutdown path we are
    // trying to avoid (see process-trheads-api.cxx for context).
    //
    BOOL WINAPI
    console_ctrl_handler (DWORD type)
    {
      switch (type)
      {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
          // 0xC000013A is STATUS_CONTROL_C_EXIT. It's the standard code
          // returned when an application is terminated by Ctrl+C.
          exit_process (0xC000013A);
          return TRUE;
      }
      return FALSE;
    }
  }
}
