#include <libiw4x/gdk/feature.hxx>

#include <libiw4x/logger.hxx>

#include <libiw4x/gdk/error.hxx>
#include <libiw4x/gdk/registry.hxx>

using namespace std;

namespace iw4x
{
  namespace gdk
  {
    char WINAPI xruntime_feature::
    is_feature_available (void*, uint32_t f) noexcept
    {
      return guard ("XGameRuntimeIsFeatureAvailable", char (0), [&] () -> char
      {
        feature x (static_cast<feature> (f));

        bool r (provides (x));

        l1 ("XGameRuntimeIsFeatureAvailable ({}) -> {}",
            name (x),
            r ? "available" : "absent");

        return r ? char (1) : char (0);
      });
    }
  }
}
