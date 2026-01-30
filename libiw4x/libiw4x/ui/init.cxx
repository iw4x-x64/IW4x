#include <libiw4x/ui/init.hxx>

#include <libiw4x/ui/ui-override.hxx>

namespace iw4x
{
  namespace ui
  {
    bool
    init ()
    {
      run_override ();

      return true;
    }
  }
}
