#include <libiw4x/iw4x.hxx>

namespace iw4x
{
  extern "C"
  {
    BOOL WINAPI
    DllMain (HINSTANCE, DWORD r, LPVOID)
    {
      if (r != DLL_PROCESS_ATTACH)
        return TRUE;

      // If we made it here, we are attached to the process.
      //
      return TRUE;
    }
  }
}
