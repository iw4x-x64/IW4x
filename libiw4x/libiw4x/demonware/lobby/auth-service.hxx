#pragma once

#include <libiw4x/demonware/lobby/auth-ticket.hxx>

#include <libiw4x/export.hxx>

namespace iw4x
{
  namespace demonware
  {
    class auth_service
    {
    public:
      auth_service ();

      static const bd_auth_ticket&
      ticket ();
    };
  }
}
