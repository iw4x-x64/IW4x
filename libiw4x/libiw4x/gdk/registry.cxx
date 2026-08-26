#include <libiw4x/gdk/registry.hxx>

#include <bit>

#include <libiw4x/logger.hxx>

#include <libiw4x/gdk/ui.hxx>
#include <libiw4x/gdk/user.hxx>
#include <libiw4x/gdk/save.hxx>
#include <libiw4x/gdk/async.hxx>
#include <libiw4x/gdk/store.hxx>
#include <libiw4x/gdk/system.hxx>
#include <libiw4x/gdk/invite.hxx>
#include <libiw4x/gdk/feature.hxx>
#include <libiw4x/gdk/networking.hxx>
#include <libiw4x/gdk/unmodelled.hxx>

namespace iw4x
{
  namespace gdk
  {
    namespace
    {
      using runtime = catalog<xasync,
                              xruntime_feature,
                              xgame_invite,
                              xgame_save,
                              xgame_ui,
                              xnetworking,
                              xstore,
                              xsystem,
                              xuser,
                              xuser_gamertag,
                              xpackage,
                              xgame_event,
                              xunidentified>;
    }

    const interface_object*
    find_interface (const guid& a, const guid& i) noexcept
    {
      return runtime::find (a, i);
    }

    bool
    provides (feature f) noexcept
    {
      return runtime::provides (f);
    }

    std::size_t
    interface_count () noexcept
    {
      return runtime::size;
    }

    unsigned
    family_count () noexcept
    {
      return static_cast<unsigned> (std::popcount (runtime::families));
    }

    void
    announce () noexcept
    {
      for (const binding& b: runtime::bindings)
        l1 ("published {}", b.object->name);

      info ("{} interfaces published, {} feature families provided",
            interface_count (),
            family_count ());
    }
  }
}
