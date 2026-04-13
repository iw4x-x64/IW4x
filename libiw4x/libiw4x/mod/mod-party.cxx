#include <libiw4x/mod/mod-party.hxx>

#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#include <libiw4x/detour.hxx>
#include <libiw4x/scheduler.hxx>

using namespace std;

namespace iw4x
{
  namespace mod
  {
    party_module::
    party_module ()
    {
      // We need to start the private party once the game actually believes we
      // are connected.
      //
      // The connectivity gate inside Live_StartPrivateParty() checks the DW
      // game state at 0x140465eb8 expects this state to be either 4 or 6.
      //
      scheduler::post (com_frame_domain,
                       []
      {
        auto state (*reinterpret_cast<int32_t*> (0x140465eb8));

        if (state == 4 || state == 6)
          Live_StartPrivateParty (0);
      },

      repeat_until_predicate {[]
      {
        auto state (*reinterpret_cast<int32_t*> (0x140465eb8));

        return state == 4 || state == 6;
      }});
    }
  }
}
