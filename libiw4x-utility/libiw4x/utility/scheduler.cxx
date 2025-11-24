#include <libiw4x/utility/scheduler.hxx>

#include <stdexcept>

#include <boost/asio/post.hpp>

#include <libiw4x/utility/xxhash.hxx>

using namespace std;

namespace iw4x
{
  namespace utility
  {
    // scheduler::strand_context
    //

    scheduler::strand_context::
    strand_context ()
      : io_context_ (),
        strand_ (io_context_)
    {
    }

    boost::asio::io_context& scheduler::strand_context::
    io_context () noexcept
    {
      return io_context_;
    }

    boost::asio::io_context::strand& scheduler::strand_context::
    strand () noexcept
    {
      return strand_;
    }

    // scheduler
    //

    scheduler::
    scheduler ()
      : strands_ (), mutex_ ()
    {
    }

    scheduler::
    ~scheduler ()
    {
      scoped_lock lck (mutex_);
      strands_.clear ();
    }

    void scheduler::
    register_strand (const string& n)
    {
      if (n.empty ())
        throw invalid_argument ("strand name cannot be empty");

      uint64_t h (xxh64 (n));
      scoped_lock lck (mutex_);

      if (strands_.contains (h))
        throw invalid_argument ("strand name already registered: " + n);

      strands_.emplace (h, make_unique<strand_context> ());
    }

    void scheduler::
    unregister_strand (const string& n)
    {
      if (n.empty ())
        throw invalid_argument ("strand name cannot be empty");

      uint64_t h (xxh64 (n));
      scoped_lock lck (mutex_);

      if (strands_.erase (h) == 0)
        throw invalid_argument ("strand not registered: " + n);
    }

    size_t scheduler::
    poll (const string& n)
    {
      auto& ctx = require_context (n).io_context ();

      // If the context ran out of work previously, it is "stopped" and we must
      // restart it to allow it to process new tasks.
      //
      if (ctx.stopped ())
        ctx.restart ();

      return ctx.poll ();
    }

    bool scheduler::
    is_registered (const string& n) const
    {
      uint64_t h (xxh64 (n));
      scoped_lock lck (mutex_);
      return strands_.contains (h);
    }

    boost::asio::io_context& scheduler::
    get_io_context (const string& n)
    {
      return require_context (n).io_context ();
    }

    boost::asio::io_context::strand& scheduler::
    get_strand (const string& n)
    {
      return require_context (n).strand ();
    }

    scheduler::strand_context& scheduler::
    require_context (const string& n)
    {
      uint64_t h (xxh64 (n));
      scoped_lock lck (mutex_);

      strand_map::iterator it (strands_.find (h));

      if (it == strands_.end ())
        throw invalid_argument ("strand not registered: " + n);

      return *it->second;
    }

    const scheduler::strand_context& scheduler::
    require_context (const string& n) const
    {
      uint64_t h (xxh64 (n));
      scoped_lock lck (mutex_);

      strand_map::const_iterator it (strands_.find (h));

      if (it == strands_.end ())
        throw invalid_argument ("strand not registered: " + n);

      return *it->second;
    }
  }
}
