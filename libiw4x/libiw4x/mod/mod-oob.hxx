#pragma once

#include <libiw4x/import.hxx>

namespace iw4x
{
  namespace mod
  {
    extern "C"
    {
      bool
      oob_dispatch (const netadr_t*, const msg_t*);
    }

    class oob_module
    {
    public:
      explicit
      oob_module ();
    };
  }
}
