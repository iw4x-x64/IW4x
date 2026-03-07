#pragma once

#include <thread>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>

#include <libnp/np-client.hxx>

#include <libiw4x/export.hxx>

namespace iw4x
{
  // Process-wide execution context.
  //
  struct LIBIW4X_SYMEXPORT context
  {
    explicit
    context ();

    // TODO: move to libnp
    //
    boost::asio::io_context
      ioc;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
      work;

    np::master_client np;
    std::jthread bg;
  };

  // `context` is required by a large portion of the detour surface and
  // therefore must be reachable where parameter injection is not possible. The
  // detour mechanism does not (yet) support passing user-defined state, and
  // introducing per-detour wrappers solely to thread this dependency would be
  // structurally redundant and unmaintainable.
  //
  // Note that ctx instance is registered during `DllMain` execution, from
  // within the game entry-point detour. Its lifetime is bound to the process
  // and is expected to remain valid for the entire duration of the
  // application.
  //
  extern LIBIW4X_SYMEXPORT context& ctx;
}
