#include <libiw4x/windows/init.hxx>

#include <libiw4x/windows/console-api.hxx>
#include <libiw4x/windows/process-threads-api.hxx>

namespace iw4x
{
  namespace windows
  {
    namespace
    {
      void (WINAPI* ExitProcessHk) (UINT) = nullptr;
    }

    bool
    init ()
    {
      ExitProcessHk = &ExitProcess;

      DWORD o (0);
      if (VirtualProtect (reinterpret_cast<void*> (ExitProcessHk),
                          0,
                          PAGE_EXECUTE_READWRITE,
                          &o) != 0)
      {
        detour (ExitProcessHk, &exit_process);
      }

      SetConsoleCtrlHandler (console_ctrl_handler, TRUE);
      return true;
    }
  }
}
