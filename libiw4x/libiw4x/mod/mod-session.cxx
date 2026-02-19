#include <libiw4x/mod/mod-session.hxx>

namespace iw4x
{
  namespace mod
  {
    session_module::
    session_module ()
      : transport_ (nullptr)
    {
      // Post the first tick.
      //
      // Note that tick() re-posts itself at the start of each invocation,
      // forming a recurring per-frame callback for the lifetime of this
      // object.
      //
      scheduler::post (com_frame_domain{}, [this] () { tick (); });
    }

    void session_module::
    tick ()
    {
      // Re-post ourselves before doing any work so that the next frame call
      // is guaranteed even if an exception unwinds through the code below.
      //
      scheduler::post (com_frame_domain{}, [this] () { tick (); });
    }
  }
}
