#pragma once

// Include Windows headers in a way that avoids the usual namespace pollution.
//
// In particular, we temporarily define WIN32_LEAN_AND_MEAN to exclude rarely
// used headers, and NOMINMAX to suppress the intrusive min() and max() macros.
//
// Note also that on MinGW, <winsock2.h> must be included before <windows.h>
// to prevent redefinition conflicts.
//
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#  ifndef NOMINMAX
#    define NOMINMAX
#    include <winsock2.h>
#    include <windows.h>
#    include <psapi.h>
#    include <d3d9.h>
#    undef NOMINMAX
#  else
#    include <winsock2.h>
#    include <windows.h>
#    include <psapi.h>
#    include <d3d9.h>
#  endif
#  undef WIN32_LEAN_AND_MEAN
#else
#  ifndef NOMINMAX
#    define NOMINMAX
#    include <winsock2.h>
#    include <windows.h>
#    include <psapi.h>
#    include <d3d9.h>
#    undef NOMINMAX
#  else
#    include <winsock2.h>
#    include <windows.h>
#    include <psapi.h>
#    include <d3d9.h>
#  endif
#endif

// Linker pseudo-variable.
//
extern "C" IMAGE_DOS_HEADER __ImageBase;
