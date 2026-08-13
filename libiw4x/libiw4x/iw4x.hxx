#pragma once

#include <windows.h>

#include <libiw4x/export.hxx>

namespace iw4x
{
  extern "C"
  {
    // Note that IW4x needs one exported symbol when it is linked with
    // MSVC.
    //
    // MinGW can produce a DLL whose export table is empty because
    // DllMain is resolved as the module entry point. MSVC rejects that
    // shape and reports that the DLL has no exports.
    //
    // Export DllMain so the same header can be used by both toolchains
    // without adding a dummy symbol.
    //
    LIBIW4X_SYMEXPORT BOOL WINAPI
    DllMain (HINSTANCE, DWORD reason, LPVOID);
  }
}
