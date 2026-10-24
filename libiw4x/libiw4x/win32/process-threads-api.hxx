// Copyright (c) the IW4x authors (see the AUTHORS file).
// SPDX-License-Identifier: GPL-3.0-only WITH AdditionRef-IW4x-Exception-1.1

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
