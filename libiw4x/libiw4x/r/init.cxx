#include <libiw4x/r/init.hxx>

#include <libiw4x/r/backend/r-d3d9.hxx>
#include <libiw4x/r/overlay/r-imgui.hxx>

namespace iw4x
{
  namespace r
  {
    bool
    init ()
    {
      create ();
      imgui ();
      return true;
    }
  }
}
