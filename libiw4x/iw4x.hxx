#include <libiw4x/windows/windows.hxx>

#include <libiw4x/export.hxx>

namespace iw4x
{
  extern "C"
  {
    LIBIW4X_SYMEXPORT BOOL WINAPI
    DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);
  }
}
