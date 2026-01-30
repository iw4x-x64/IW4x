#pragma once

#include <libiw4x/import.hxx>
#include <libiw4x/export.hxx>

namespace iw4x
{
  namespace windows
  {
    LIBIW4X_SYMEXPORT BOOL WINAPI
    console_ctrl_handler (DWORD type);
  }
}
