#pragma once

#include <windows.h>

#include <libiw4x/export.hxx>

namespace iw4x
{
  extern "C"
  {
    // DllMain normally doesn't need to be exported. The loader finds it
    // from the PE entry point and, with MinGW, that is enough to
    // produce a DLL with no ordinary exports at all.
    //
    // That becomes a problem when the resulting import library is
    // consumed by MSVC, which doesn't accept this particular shape of
    // DLL and complains that there is nothing to export.
    //
    // So export DllMain as well. We already have a real symbol with the
    // right linkage and lifetime, and doing this keeps the MinGW and
    // MSVC builds on the same interface without inventing an otherwise
    // useless export just to satisfy the latter.
    //
    LIBIW4X_SYMEXPORT BOOL WINAPI
    DllMain (HINSTANCE, DWORD reason, LPVOID);
  }

  // Return the libiw4x module handle captured by DllMain during process
  // attachment.
  //
  // Note that this is deliberately recorded while Windows is handing us
  // the HINSTANCE instead of rediscovering the module later. Quite a
  // few places need to refer back to the DLL itself and, once
  // attachment has completed, this gives them the answer without
  // another module lookup.
  //
  HMODULE
  module () noexcept;
}

// Linker pseudo-variable.
//
extern "C" IMAGE_DOS_HEADER __ImageBase;
