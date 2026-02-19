#pragma once

#include <libiw4x/session/machine.hxx>
#include <libiw4x/session/domain.hxx>

#include <libiw4x/transport/transport.hxx>

#include <libiw4x/import.hxx>
#include <libiw4x/export.hxx>

namespace iw4x
{
  namespace mod
  {
    class LIBIW4X_SYMEXPORT session_module
      : public session::state_change_observer
    {
    public:
      explicit
      session_module ();

      // Create a new hosted session.
      //
      // Note that the local peer becomes the authoritative host.
      //
      void
      create_session (session::host_role,
                      session::session_metadata);

      // Drive all pending session events and transport I/O for one frame.
      //
      void
      tick ();

    private:
      unique_ptr<transport::transport_capability> transport_;
    };
  }
}
