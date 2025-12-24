#include <libiw4x/windows/init.hxx>

#include <libiw4x/windows/process-threads-api.hxx>

namespace iw4x
{
  namespace windows
  {
    namespace
    {
      void (WINAPI* ExitProcessHk) (UINT) = nullptr;
    }

    void
    init ()
    {
      ExitProcessHk = &ExitProcess;

      detour (ExitProcessHk, &exit_process);
    }
  }
}
