#pragma once

#include <utility>

#include <quill/LogFunctions.h>
#include <quill/Logger.h>

namespace iw4x
{
  namespace logger::detail
  {
    extern quill::Logger* logger;

    template <quill::LogLevel L, typename... A>
    struct statement
    {
      statement (
        const char* format,
        A&&... args,
        quill::SourceLocation location = quill::SourceLocation::current ())
      {
        // So what we are doing here is using construction as the
        // logging statement. In other words, given
        //
        //   info ("starting {}", name);
        //
        // `info` is a temporary object whose constructor does all the
        // work. It may look a little unusual at first but gives us the
        // syntax we want without having to put a macro in front of
        // every logging call.
        //
        // Most of the machinery lives in this base class since all our
        // public statements differ only in the Quill level. In
        // particular, keeping the forwarding here is important since
        // otherwise every new level would have to reproduce this
        // constructor exactly.
        //
        // Note also the slightly subtle source location business. We
        // want the location of
        //
        //   info ("...");
        //
        // and not the location of quill::log() below. For that to work
        // the default argument has to be evaluated while constructing
        // the public statement and then carried with us into this
        // common implementation. Moving SourceLocation::current() into
        // the body would therefore quietly make every message point
        // here, which is generally the opposite of what somebody
        // looking at a log wants.
        //
        // The argument types have already been deduced by the public
        // statement at this point. Preserve them as they came in when
        // passing them on to Quill. This matters for lvalues in
        // particular since A can itself be a reference type.
        //
        // There is no tag associated with these messages for now, hence
        // the empty string. The level already carries the distinction
        // we care about here and putting fail/info/l1 and so on into
        // the message would only duplicate it.
        //
        // Finally, use the generic Quill entry point since the level is
        // a template argument. This is the one place where our
        // statement types are translated into Quill logging calls.
        //
        quill::log (logger,
                    "",
                    L,
                    format,
                    location,
                    std::forward<A> (args)...);
      }
    };
  }

  // Note that we need the deduction guide even though the constructor
  // itself is inherited. The arguments belong to the class template and
  // have to be known before that constructor can be selected.

  template <typename... A>
  struct fail : logger::detail::statement<quill::LogLevel::Error, A...>
  {
    using logger::detail::statement<quill::LogLevel::Error, A...>::statement;
  };

  template <typename... A>
  fail (const char*, A&&...) -> fail<A...>;

  template <typename... A>
  struct warn : logger::detail::statement<quill::LogLevel::Warning, A...>
  {
    using logger::detail::statement<quill::LogLevel::Warning, A...>::statement;
  };

  template <typename... A>
  warn (const char*, A&&...) -> warn<A...>;

  template <typename... A>
  struct info : logger::detail::statement<quill::LogLevel::Info, A...>
  {
    using logger::detail::statement<quill::LogLevel::Info, A...>::statement;
  };

  template <typename... A>
  info (const char*, A&&...) -> info<A...>;

  template <typename... A>
  struct l1 : logger::detail::statement<quill::LogLevel::TraceL1, A...>
  {
    using logger::detail::statement<quill::LogLevel::TraceL1, A...>::statement;
  };

  template <typename... A>
  l1 (const char*, A&&...) -> l1<A...>;

  template <typename... A>
  struct l2 : logger::detail::statement<quill::LogLevel::TraceL2, A...>
  {
    using logger::detail::statement<quill::LogLevel::TraceL2, A...>::statement;
  };

  template <typename... A>
  l2 (const char*, A&&...) -> l2<A...>;

  template <typename... A>
  struct l3 : logger::detail::statement<quill::LogLevel::TraceL3, A...>
  {
    using logger::detail::statement<quill::LogLevel::TraceL3, A...>::statement;
  };

  template <typename... A>
  l3 (const char*, A&&...) -> l3<A...>;
}
