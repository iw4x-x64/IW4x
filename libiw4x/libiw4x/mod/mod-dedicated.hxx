#pragma once

#include <libiw4x/import.hxx>
#include <libiw4x/export.hxx>

namespace iw4x
{
  namespace mod
  {
    class dedicated_module
    {
    public:
      dedicated_module ();

      static bool
      is_dedicated ();
    };
  }
}
