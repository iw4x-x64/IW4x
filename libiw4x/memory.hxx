#pragma once

#include <libiw4x/iw4x.hxx>
#include <libiw4x/export.hxx>

namespace iw4x
{
  LIBIW4X_SYMEXPORT void*
  memwrite (void* dest, int ch, size_t count);

  LIBIW4X_SYMEXPORT void*
  memwrite (uintptr_t dest, int ch, size_t count);

  LIBIW4X_SYMEXPORT void*
  memwrite (void* dest, int ch, size_t count);

  LIBIW4X_SYMEXPORT void*
  memwrite (uintptr_t dest, int ch, size_t count);

  LIBIW4X_SYMEXPORT void*
  memwrite (void* dest, const void* src, size_t count);

  LIBIW4X_SYMEXPORT void*
  memwrite (uintptr_t dest, const void* src, size_t count);

  template <size_t N>
  inline void*
  memwrite (uintptr_t dest, const uint8_t (&payload) [N])
  {
    return memwrite (dest, payload, N);
  }

  template <size_t N>
  inline void*
  memwrite (uintptr_t dest, const char (&payload) [N])
  {
    // Note that for string literals, N includes the null terminator,
    // so we use N - 1 to strip it.
    //
    return memwrite (dest, payload, N - 1);
  }
}
