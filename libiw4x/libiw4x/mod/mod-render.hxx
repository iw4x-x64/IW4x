#pragma once

#include <libiw4x/import.hxx>
#include <libiw4x/export.hxx>

namespace iw4x
{
  namespace mod
  {
    class LIBIW4X_SYMEXPORT render
    {
    public:
      render ();

      static void
      create ();

      static void
      imgui ();

      static void
      on_frame (function<void (IDirect3DDevice9*)> cb);

      static void
      on_reset (function<void ()> lost, function<void ()> reset);
    };
  }
}
