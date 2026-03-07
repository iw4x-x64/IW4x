#include <libiw4x/context.hxx>

namespace iw4x
{
  context::
  context ()
    : work (boost::asio::make_work_guard (ioc)),
      np (ioc, np::client_config {"147.135.10.99", "28961"}),
      bg ([this] () { ioc.run (); })
  {
    // Intentionally left empty by design
    //
  }
}
