#include <libiw4x/xcurl/runtime.hxx>

#include <libiw4x/gdk/user.hxx>

#include <libiw4x/xcurl/local.hxx>

using namespace std;

namespace iw4x
{
  namespace xcurl
  {
    void
    install () noexcept
    {
      gdk::serve (&served);
    }
  }
}
