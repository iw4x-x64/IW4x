#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

// Temp 
//
#define WINVER       0x0A00
#define _WIN32_WINNT 0x0A00

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>

#include <libiw4x/utility/export.hxx>

namespace iw4x
{
  namespace utility
  {
    class LIBIW4X_UTILITY_SYMEXPORT scheduler
    {
    public:
      scheduler ();
      ~scheduler ();

      scheduler (const scheduler&) = delete;
      scheduler& operator= (const scheduler&) = delete;

      // Register a new strand with the given name.
      //
      // Strand names must be unique. Throws std::invalid_argument if the
      // name is empty or already registered.
      //
      void
      register_strand (const std::string& name);

      // Unregister a strand.
      //
      // Throws std::invalid_argument if the strand is not registered.
      //
      void
      unregister_strand (const std::string& name);

      // Poll one strand to process pending tasks.
      //
      // This is a convenience wrapper that looks up the strand by name and
      // calls io_context::poll() on its underlying context. Returns the
      // number of handlers executed. Throws std::invalid_argument if the
      // strand is not registered.
      //
      std::size_t
      poll (const std::string& name);

      // Post a task to the specified strand.
      //
      // This is a convenience wrapper that looks up the strand by name and
      // posts the task using boost::asio::post(). Throws std::invalid_argument
      // if the strand is not registered.
      //
      template <typename F> void
      post (const std::string& strand_name, F&& function);

      // Check if a strand is registered.
      //
      bool
      is_registered (const std::string& name) const;

      // Get direct access to the strand's io_context.
      //
      // Returns a reference to the underlying io_context for the named strand.
      // Throws std::invalid_argument if the strand is not registered.
      //
      boost::asio::io_context&
      get_io_context (const std::string& name);

      // Get direct access to the strand.
      //
      // Returns a reference to the underlying strand for the named strand.
      // Throws std::invalid_argument if the strand is not registered.
      //
      boost::asio::io_context::strand&
      get_strand (const std::string& name);

    private:
      class strand_context
      {
      public:
        strand_context ();
        ~strand_context () = default;

        strand_context (const strand_context&) = delete;
        strand_context& operator= (const strand_context&) = delete;

        boost::asio::io_context&
        io_context () noexcept;

        boost::asio::io_context::strand&
        strand () noexcept;

      private:
        boost::asio::io_context io_context_;
        boost::asio::io_context::strand strand_;
      };

      using strand_map = std::unordered_map<std::uint64_t,
                                            std::unique_ptr<strand_context>>;

      strand_map strands_;
      mutable std::mutex mutex_;

      strand_context&
      require_context (const std::string& name);

      const strand_context&
      require_context (const std::string& name) const;
    };

    // Template implementation
    //
    template <typename F>
    void scheduler::
    post (const std::string& strand_name, F&& function)
    {
      boost::asio::io_context::strand& s (get_strand (strand_name));
      boost::asio::post (s, std::forward<F> (function));
    }
  }
}
