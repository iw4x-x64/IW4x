#pragma once

#include <quill/LogFunctions.h>
#include <quill/Logger.h>
#include <quill/backend/BackendUtilities.h>
#include <quill/core/LogLevel.h>
#include <quill/core/SourceLocation.h>

#include <libiw4x/log/category.hxx>
#include <libiw4x/log/severity.hxx>

#include <libiw4x/export.hxx>

namespace iw4x
{
  class LIBIW4X_SYMEXPORT logger
  {
  public:
    logger ();
    ~logger ();

    logger (const logger&) = delete;
    logger& operator = (const logger&) = delete;

    logger (logger&&) = delete;
    logger& operator = (logger&&) = delete;

    // Flush all pending log messages synchronously.
    //
    void
    flush () noexcept;
  };

  // Process-lifetime logger instance.
  //
  LIBIW4X_SYMEXPORT extern logger* active_logger;

  namespace log
  {
    // Use structs rather than plain functions for the logging frontend.
    //
    // The problem we are trying to solve here is capturing the call site's
    // source location automatically without resorting to macros. Namely, we
    // need to accept a variadic number of formatting arguments, but we cannot
    // express this in a well-formed function signature since the default
    // argument for the source location would have to come after the parameter
    // pack. Struct constructor on the other hand can take the parameter pack,
    // with the source location tacked on as a trailing default argument.
    //
    // Note that this relies on CTAD to properly deduce the template types.

    template <log_category C, typename... A>
    struct trace_l3
    {
      trace_l3 (C,
                char const* f,
                A&&... a,
                quill::SourceLocation l = quill::SourceLocation::current ())
      {
        if constexpr (quill::LogLevel::TraceL3 >= compiled_minimum_level)
        {
          quill::Logger* q (category_logger<C> ());

          if (q && q->should_log_statement (quill::LogLevel::TraceL3))
          {
            quill::log (q, "", quill::LogLevel::TraceL3, f, l,
                        static_cast<A&&> (a)...);
          }
        }
      }
    };

    template <log_category C, typename... A>
    trace_l3 (C, char const*, A&&...) -> trace_l3<C, A...>;


    template <log_category C, typename... A>
    struct trace_l2
    {
      trace_l2 (C,
                char const* f,
                A&&... a,
                quill::SourceLocation l = quill::SourceLocation::current ())
      {
        if constexpr (quill::LogLevel::TraceL2 >= compiled_minimum_level)
        {
          quill::Logger* q (category_logger<C> ());

          if (q && q->should_log_statement (quill::LogLevel::TraceL2))
          {
            quill::log (q, "", quill::LogLevel::TraceL2, f, l,
                        static_cast<A&&> (a)...);
          }
        }
      }
    };

    template <log_category C, typename... A>
    trace_l2 (C, char const*, A&&...) -> trace_l2<C, A...>;


    template <log_category C, typename... A>
    struct trace_l1
    {
      trace_l1 (C,
                char const* f,
                A&&... a,
                quill::SourceLocation l = quill::SourceLocation::current ())
      {
        if constexpr (quill::LogLevel::TraceL1 >= compiled_minimum_level)
        {
          quill::Logger* q (category_logger<C> ());

          if (q && q->should_log_statement (quill::LogLevel::TraceL1))
          {
            quill::log (q, "", quill::LogLevel::TraceL1, f, l,
                        static_cast<A&&> (a)...);
          }
        }
      }
    };

    template <log_category C, typename... A>
    trace_l1 (C, char const*, A&&...) -> trace_l1<C, A...>;


    template <log_category C, typename... A>
    struct debug
    {
      debug (C,
             char const* f,
             A&&... a,
             quill::SourceLocation l = quill::SourceLocation::current ())
      {
        if constexpr (quill::LogLevel::Debug >= compiled_minimum_level)
        {
          quill::Logger* q (category_logger<C> ());

          if (q && q->should_log_statement (quill::LogLevel::Debug))
          {
            quill::log (q, "", quill::LogLevel::Debug, f, l,
                        static_cast<A&&> (a)...);
          }
        }
      }
    };

    template <log_category C, typename... A>
    debug (C, char const*, A&&...) -> debug<C, A...>;


    template <log_category C, typename... A>
    struct info
    {
      info (C,
            char const* f,
            A&&... a,
            quill::SourceLocation l = quill::SourceLocation::current ())
      {
        if constexpr (quill::LogLevel::Info >= compiled_minimum_level)
        {
          quill::Logger* q (category_logger<C> ());

          if (q && q->should_log_statement (quill::LogLevel::Info))
          {
            quill::log (q, "", quill::LogLevel::Info, f, l,
                        static_cast<A&&> (a)...);
          }
        }
      }
    };

    template <log_category C, typename... A>
    info (C, char const*, A&&...) -> info<C, A...>;


    template <log_category C, typename... A>
    struct notice
    {
      notice (C,
              char const* f,
              A&&... a,
              quill::SourceLocation l = quill::SourceLocation::current ())
      {
        if constexpr (quill::LogLevel::Notice >= compiled_minimum_level)
        {
          quill::Logger* q (category_logger<C> ());

          if (q && q->should_log_statement (quill::LogLevel::Notice))
          {
            quill::log (q, "", quill::LogLevel::Notice, f, l,
                        static_cast<A&&> (a)...);
          }
        }
      }
    };

    template <log_category C, typename... A>
    notice (C, char const*, A&&...) -> notice<C, A...>;


    template <log_category C, typename... A>
    struct warning
    {
      warning (C,
               char const* f,
               A&&... a,
               quill::SourceLocation l = quill::SourceLocation::current ())
      {
        if constexpr (quill::LogLevel::Warning >= compiled_minimum_level)
        {
          quill::Logger* q (category_logger<C> ());

          if (q && q->should_log_statement (quill::LogLevel::Warning))
          {
            quill::log (q, "", quill::LogLevel::Warning, f, l,
                        static_cast<A&&> (a)...);
          }
        }
      }
    };

    template <log_category C, typename... A>
    warning (C, char const*, A&&...) -> warning<C, A...>;


    template <log_category C, typename... A>
    struct error
    {
      error (C,
             char const* f,
             A&&... a,
             quill::SourceLocation l = quill::SourceLocation::current ())
      {
        if constexpr (quill::LogLevel::Error >= compiled_minimum_level)
        {
          quill::Logger* q (category_logger<C> ());

          if (q && q->should_log_statement (quill::LogLevel::Error))
          {
            quill::log (q, "", quill::LogLevel::Error, f, l,
                        static_cast<A&&> (a)...);
          }
        }
      }
    };

    template <log_category C, typename... A>
    error (C, char const*, A&&...) -> error<C, A...>;


    template <log_category C, typename... A>
    struct critical
    {
      critical (C,
                char const* f,
                A&&... a,
                quill::SourceLocation l = quill::SourceLocation::current ())
      {
        if constexpr (quill::LogLevel::Critical >= compiled_minimum_level)
        {
          quill::Logger* q (category_logger<C> ());

          if (q && q->should_log_statement (quill::LogLevel::Critical))
          {
            quill::log (q, "", quill::LogLevel::Critical, f, l,
                        static_cast<A&&> (a)...);

            // Flush the log immediately on critical errors.
            //
            // That is, If we're logging at this severity, there's a good chance
            // of an impending abort, so we want the message to makes it out of
            // Quill's thread-local buffer and be written out before the process
            // dies.
            //
            q->flush_log ();
          }
        }
      }
    };

    template <log_category C, typename... A>
    critical (C, char const*, A&&...) -> critical<C, A...>;
  }
}
