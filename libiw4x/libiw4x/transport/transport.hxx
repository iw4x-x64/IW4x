#pragma once

namespace iw4x
{
  namespace transport
  {
    struct transport_capability
    {
      virtual
      ~transport_capability () = default;

      // Drive internal transport state for one frame.
      //
      virtual void
      tick () = 0;
    };
  }
}
