#include <libiw4x/iw/live-win.hxx>

namespace iw4x
{
  namespace iw
  {
    bool
    live_start_signin_any (unsigned int)
    {
      return true;
    }

    LIBIW4X_SYMEXPORT const char*
    live_get_local_client_name (int)
    {
      return "Hello";
    }
  }
}
