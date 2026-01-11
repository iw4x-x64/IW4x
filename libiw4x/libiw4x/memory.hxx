#pragma once

#include <libiw4x/iw4x.hxx>
#include <libiw4x/export.hxx>

namespace iw4x
{
  // Fill memory with a value.
  //
  // Similar to memset, but if the value is 0x90 (NOP), we try to replace
  // runs of single-byte NOPs with optimal multi-byte architectural NOP
  // sequences to relieve the instruction decoder.
  //
  LIBIW4X_SYMEXPORT void*
  memwrite (void* dest, int ch, size_t count);

  LIBIW4X_SYMEXPORT void*
  memwrite (uintptr_t dest, int ch, size_t count);

  // Copy memory.
  //
  // Similar to memcpy, but with the same NOP-optimization logic: if the
  // source buffer contains runs of 0x90, we expand them into multi-byte
  // NOPs in the destination.
  //
  LIBIW4X_SYMEXPORT void*
  memwrite (void* dest, const void* src, size_t count);

  LIBIW4X_SYMEXPORT void*
  memwrite (uintptr_t dest, const void* src, size_t count);

  // Convenience wrappers for static arrays.
  //
  template <size_t N>
  inline void*
  memwrite (uintptr_t dest, const uint8_t (&payload)[N])
  {
    return memwrite (dest, payload, N);
  }

  template <size_t N>
  inline void*
  memwrite (uintptr_t dest, const char (&payload)[N])
  {
    // If we are passing a string literal (which is the 99% use case here),
    // N includes the null terminator. Since we are patching code, we almost
    // never want to write that trailing null byte.
    //
    // So we strip it. If we really want to write a null byte, we can use the
    // raw pointer overload or a uint8_t array.
    //
    return memwrite (dest, payload, N - 1);
  }
}
