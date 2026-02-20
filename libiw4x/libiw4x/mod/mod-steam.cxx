#include <libiw4x/mod/mod-steam.hxx>

#include <libiw4x/logger.hxx>

#include <steam_api.h>
#include <steam_api_common.h>

namespace iw4x
{
  namespace mod
  {
    steam_module::
    steam_module ()
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
