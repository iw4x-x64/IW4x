#pragma once

namespace iw4x
{
  namespace session
  {
    // Observer interface for session state changes.
    //
    // The idea is to decouple listeners from the machine's internals.
    //
    struct state_change_observer
    {
      virtual
      ~state_change_observer () = default;
    };
  }
}
