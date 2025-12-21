#pragma once

#include <libiw4x/iw4x.hxx>
#include <libiw4x/export.hxx>

namespace iw4x
{
  LIBIW4X_SYMEXPORT void*
  memwrite (void* dest, int ch, size_t count);

  LIBIW4X_SYMEXPORT void*
  memwrite (uintptr_t dest, int ch, size_t count);
}
