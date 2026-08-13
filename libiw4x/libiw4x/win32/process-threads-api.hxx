#pragma once

#include <windows.h>

namespace iw4x
{
  namespace win32
  {
    LPVOID*
    exit_process_original ();

    void WINAPI
    exit_process (UINT);
  }
}
