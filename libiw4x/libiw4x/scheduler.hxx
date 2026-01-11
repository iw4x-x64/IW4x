#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>

#include <libiw4x/iw4x.hxx>
#include <libiw4x/export.hxx>

namespace iw4x
{
  // The central scheduler for the IW4x modification.
  //
  // The idea here is to wrap the Boost.Asio io_context to provide a named
  // "strand" registry and decouple call sites (who just need to know the name,
  // e.g., "networking", "logic") from the lifetime management of the actual
  // execution contexts.
  //
  class LIBIW4X_SYMEXPORT scheduler
  {
  public:
    using strand_type =
      boost::asio::strand<boost::asio::io_context::executor_type>;

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

    // Destroy a named strand.
    //
    // Note that this does not cancel pending handlers associated with this
    // strand, but simply removes the reference from the map. Return false
    // if the name is not found.
    //
    bool
    destroy (const string& name);

    // Post a unit of work to the specified named strand.
    //
    // Return false if the strand hasn't been created yet. We forward the
    // work handler to avoid unnecessary copies.
    //
    template <typename F>
    bool
    post (const string& name, F&& work)
    {
      strand_type* s (find (name));

      if (s == nullptr)
        return false;

      boost::asio::post (*s, forward<F> (work));
      return true;
    }

    // Register a task to be executed cyclically on a named strand.
    //
    // Unlike a timer, these tasks are driven by the poll() heartbeat. They
    // are posted for execution exactly once every time we poll this specific
    // strand (or the global context).
    //
    // Useful for game-loop logic that needs to run "every tick".
    //
    template <typename F>
    bool
    loop (const string& name, F&& work)
    {
      // We can't loop on a strand that doesn't exist.
      //
      if (!exists (name))
        return false;

      loops[name].emplace_back (forward<F> (work));
      return true;
    }

    // Drive the execution context.
    //
    // This pumps the underlying io_context. If a specific name is provided,
    // we only inject the 'loop' tasks for that specific strand before
    // polling.
    //
    // Note: If name is empty/omitted, we execute the loop tasks for ALL
    // registered strands.
    //
    void
    poll (const string& name = string ());

    // Check if a named strand is currently registered.
    //
    bool
    exists (const string& name) const;

  private:
    // Helper to look up a strand without exposing the map iterator. Returns
    // nullptr if not found.
    //
    strand_type*
    find (const string& name);

    const strand_type*
    find (const string& name) const;

  private:
    unique_ptr<boost::asio::io_context> context;
    unordered_map<string, strand_type> strands;

    // Tasks that need to run on every poll cycle, grouped by strand name.
    //
    unordered_map<string, vector<function<void ()>>> loops;
  };
}
