#include <libiw4x/gdk/types.hxx>

using namespace std;

namespace iw4x
{
  namespace gdk
  {
    const char*
    name (feature f) noexcept
    {
      switch (f)
      {
      case feature::accessibility:            return "XAccessibility";
      case feature::app_capture:              return "XAppCapture";
      case feature::async:                    return "XAsync";
      case feature::async_provider:           return "XAsyncProvider";
      case feature::display:                  return "XDisplay";
      case feature::game:                     return "XGame";
      case feature::game_invite:              return "XGameInvite";
      case feature::game_save:                return "XGameSave";
      case feature::game_ui:                  return "XGameUi";
      case feature::launcher:                 return "XLauncher";
      case feature::networking:               return "XNetworking";
      case feature::package:                  return "XPackage";
      case feature::persistent_local_storage: return "XPersistentLocalStorage";
      case feature::speech_synthesizer:       return "XSpeechSynthesizer";
      case feature::store:                    return "XStore";
      case feature::system:                   return "XSystem";
      case feature::task_queue:               return "XTaskQueue";
      case feature::thread:                   return "XThread";
      case feature::user:                     return "XUser";
      case feature::error:                    return "XError";
      case feature::game_event:               return "XGameEvent";
      case feature::game_streaming:           return "XGameStreaming";
      }

      return "<unrecovered family>";
    }
  }
}
