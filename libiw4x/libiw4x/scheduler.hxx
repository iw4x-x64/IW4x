#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>

#include <libiw4x/export.hxx>

namespace iw4x
{
  // NOTE:
  //
  // We use Boost.Asio as the backing scheduler. Note that this is not exactly
  // a perfect fit: Asio is fundamentally an I/O reactor designed for
  // multiplexing, whereas we need a set of independently tickable, logical
  // schedulers.
  //
  // That is, we are layering our specific isolation semantics on top of
  // their threading model. It works, but creates some friction (and overhead)
  // regarding strand semantics vs. true isolation.
  //
  // TODO:
  //
  // Replace with sender/receiver-based implementation that more directly
  // models the execution semantics we intend to expose.
  //
  // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2300r10.html
  //
  class LIBIW4X_SYMEXPORT scheduler
  {
    using exec_t = boost::asio::io_context::executor_type;
    using strand_t = boost::asio::strand<exec_t>;

  public:
    scheduler ();
    ~scheduler ();

    scheduler (const scheduler&) = delete;
    scheduler& operator= (const scheduler&) = delete;

    // Create a new named execution strand.
    //
    // Return false if a strand with this name is already registered. We
    // treat name collision as a soft error since the caller might want to
    // recover or ignore it.
    //
    bool
    create (const string& name);

    // Post a unit of work to the specified named strand.
    //
    // Return false if the strand hasn't been created yet. We forward the
    // work handler to avoid unnecessary copies.
    //
    template <typename F>
    bool
    post (const string& name, F&& work)
    {
      const strand_t* s (find (name));

      if (s == nullptr)
        return false;

      boost::asio::post (*s, forward<F> (work));
      return true;
    }

    // Drive the execution context.
    //
    void
    poll (const string& name);

  private:
    // Helper to look up a strand without exposing the map iterator. Returns
    // nullptr if not found.
    //
    const strand_t*
    find (const string& name) const;

  private:
    unique_ptr<boost::asio::io_context> context;
    unordered_map<string, strand_t> strands;
  };
}
