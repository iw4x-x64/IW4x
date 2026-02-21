#include <libiw4x/mod/mod-session.hxx>

#include <libiw4x/transport/steam.hxx>

namespace iw4x
{
  namespace mod
  {
    session_module::
    session_module ()
      : transport_ (nullptr)
    {
      scheduler::post (com_frame_domain {},
                       [this] ()
      {
        tick ();
      }, repeat_every_tick {});
    }

    void session_module::
    create_session (session::host_role, session::session_metadata m)
    {
       transport_ = make_unique<steam_transport> ();
    }

    void session_module::
    tick ()
    {
      // Tick the transport first.
      //
      if (transport_)
        transport_->tick ();
    }
  }
}
