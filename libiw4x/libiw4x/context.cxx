#include <libiw4x/context.hxx>

#include <libiw4x/scheduler.hxx>

namespace iw4x
{
  context::context (scheduler& s, logger* l)
    : sched (s), log (l)
  {
    // Intentionally left empty by design
    //
  }
}
