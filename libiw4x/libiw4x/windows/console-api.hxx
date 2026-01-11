#pragma once

#include <libiw4x/import.hxx>
#include <libiw4x/export.hxx>

namespace iw4x
{
  namespace windows
  {
    // Handler routine for SetConsoleCtrlHandler.
    //
    // We need this to catch console events (Ctrl+C, Close, etc.) and force our
    // custom exit path. If we don't, the default handler will trigger the
    // CRT shutdown sequence which is generally unsafe for us.
    //
    LIBIW4X_SYMEXPORT BOOL WINAPI
    console_ctrl_handler (DWORD type);
  }
}
