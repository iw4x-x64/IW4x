#pragma once

#include <libiw4x/export.hxx>
#include <libiw4x/import.hxx>
#include <libiw4x/scheduler.hxx>

namespace iw4x
{
  namespace console
  {

    LIBIW4X_SYMEXPORT void
    Con_ToggleConsole ();

    LIBIW4X_SYMEXPORT void
    init (scheduler&);
  }
}
