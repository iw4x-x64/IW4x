#pragma once

#include <libiw4x/export.hxx>

namespace iw4x
{
  extern "C"
  {
    // MinGW is happy to produce a DLL with no exports, treating DllMain as a
    // special entry point. MSVC, however, refuses to link if the export table
    // is empty. So we explicitly export DllMain to satisfy the latter.
    //
    LIBIW4X_SYMEXPORT BOOL WINAPI
    DllMain (HINSTANCE, DWORD r, LPVOID);
  }
}
