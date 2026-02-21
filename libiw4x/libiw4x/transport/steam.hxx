#pragma once

#include <libiw4x/transport/transport.hxx>

namespace iw4x
{
  namespace transport
  {
    class steam_transport : public transport_capability
    {
    public:
      steam_transport ();

      void
      tick () override;
    };
  }
}
