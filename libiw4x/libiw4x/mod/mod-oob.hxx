#pragma once

#include <libiw4x/import.hxx>

namespace iw4x
{
  namespace mod
  {
    extern "C"
    {
      bool
      oob_dispatch (const network_address*, const message*);
    }

    class oob_module
    {
    public:
      explicit
      oob_module ();
    };
  }
}
