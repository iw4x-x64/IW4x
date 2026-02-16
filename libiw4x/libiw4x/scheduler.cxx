#include <libiw4x/scheduler.hxx>

using namespace boost::asio;

namespace iw4x
{
  scheduler::scheduler ()
    : context (make_unique<io_context> ())
  {
    // Intentionally left empty by design
    //
  }

  scheduler::~scheduler ()
  {
    // Intentionally left empty by design
    //
  }

  bool
  scheduler::create (const string& n)
  {
    // We treat existence as a failure to create because we expect unique
    // names.
    //
    auto r (strands.try_emplace (n, make_strand (*context)));
    return r.second;
  }

  void
  scheduler::poll (const string& n)
  {
    const strand_type* s (find (n));

    // If the strand doesn't exist, we can't really post tasks to it. We could
    // theoretically poll the global context anyway, but it's safer to bail
    // out to avoid masking logic errors in the caller.
    //
    if (s == nullptr)
      return;

    // Now drive the context.
    //
    // If the context has run out of work (stopped), we need to kick it back
    // to life before we can poll again. Otherwise, the tasks we just posted
    // will sit in the queue forever.
    //
    if (context->stopped ())
      context->restart ();

    // Note that this runs *all* ready handlers on the context, not just the
    // ones for our strand.
    //
    context->poll ();
  }

  const scheduler::strand_type*
  scheduler::find (const string& n) const
  {
    auto i (strands.find (n));
    return i != strands.end () ? &i->second : nullptr;
  }
}
