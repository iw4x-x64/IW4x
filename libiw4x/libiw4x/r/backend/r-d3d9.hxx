#pragma once

#include <libiw4x/import.hxx>
#include <libiw4x/export.hxx>

namespace iw4x
{
  namespace r
  {
    LIBIW4X_SYMEXPORT void
    create ();

    LIBIW4X_SYMEXPORT void
    on_frame (function<void (IDirect3DDevice9*)> cb);

    LIBIW4X_SYMEXPORT void
    on_reset (function<void ()> lost, function<void ()> reset);
  }
}
