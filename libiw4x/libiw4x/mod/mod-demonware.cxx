#include <libiw4x/mod/mod-demonware.hxx>

#include <algorithm>
#include <cstdint>

#include <libiw4x/detour.hxx>
#include <libiw4x/demonware/bd-platform-log.hxx>

using namespace std;

namespace iw4x
{

  namespace mod
  {
    demonware_module::
    demonware_module ()
    {
      detour (bdLogMessage, &demonware::bd_log_message);
    }
  }
}
