#include <libiw4x/steam/steam.hxx>

#include <libiw4x/logger.hxx>

#include <steam_api.h>
#include <steam_api_common.h>

namespace iw4x
{
  namespace steam
  {
    steam::
    steam ()
    {
      SteamErrMsg e {};
      ESteamAPIInitResult r (SteamAPI_InitFlat (&e));

      if (r != k_ESteamAPIInitResult_OK)
      {
        log::warning {categories::steam {}, "SteamAPI_InitFlat failed: {}", e};
        return;
      }
    }
  }
}
