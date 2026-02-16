#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>

#include <libiw4x/export.hxx>

namespace iw4x
{
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

    // Post a unit of work to the specified named strand.
    //
    // Return false if the strand hasn't been created yet. We forward the
    // work handler to avoid unnecessary copies.
    //
    template <typename F>
    bool
    post (const string& name, F&& work)
    {
      const strand_type* s (find (name));

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
    const strand_type*
    find (const string& name) const;

  private:
    unique_ptr<boost::asio::io_context> context;
    unordered_map<string, strand_type> strands;
  };
}
