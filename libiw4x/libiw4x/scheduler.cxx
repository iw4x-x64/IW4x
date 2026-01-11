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

  bool
  scheduler::destroy (const string& n)
  {
    // Clean up the loop registry first.
    //
    // Note that we ignore the result here: whether there were loops attached
    // or not doesn't determine the success of the 'destroy' operation. We
    // just want to ensure that *if* there were any, they are gone.
    //
    loops.erase (n);

    // The operation is considered successful only if the strand itself
    // existed and was removed.
    //
    return strands.erase (n) != 0;
  }

  void
  scheduler::poll (const string& n)
  {
    strand_type* s (find (n));

    // If the strand doesn't exist, we can't really post tasks to it. We could
    // theoretically poll the global context anyway, but it's safer to bail
    // out to avoid masking logic errors in the caller.
    //
    if (s == nullptr)
      return;

    // Inject the recurring "loop" tasks for this specific strand.
    //
    if (auto l (loops.find (n)); l != loops.end ())
    {
      for (const auto& task : l->second)
        boost::asio::post (*s, task);
    }

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

  bool
  scheduler::exists (const string& n) const
  {
    return find (n) != nullptr;
  }

  scheduler::strand_type*
  scheduler::find (const string& n)
  {
    auto i (strands.find (n));
    return i != strands.end () ? &i->second : nullptr;
  }

  const scheduler::strand_type*
  scheduler::find (const string& n) const
  {
    auto i (strands.find (n));
    return i != strands.end () ? &i->second : nullptr;
  }
}
