#pragma once

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/LogFunctions.h>
#include <quill/Logger.h>

#include <quill/backend/BackendUtilities.h>

#include <quill/core/LogLevel.h>
#include <quill/core/SourceLocation.h>

#include <quill/sinks/ConsoleSink.h>
#include <quill/sinks/RotatingFileSink.h>

#include <libiw4x/log/log-category.hxx>
#include <libiw4x/log/log-severity.hxx>

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
  };

  LIBIW4X_SYMEXPORT extern logger* active_logger;

  namespace log
  {
    // Rate-limiting gate for a log call site.
    //
    // Returns true at most once per interval. All intermediate checks return
    // false.
    //
    struct rate_limiter
    {
      using clock = std::chrono::steady_clock;
      using time_point = clock::time_point;
      using duration = clock::duration;

      explicit
      rate_limiter (duration d) noexcept
        : interval_ (d), last_ (time_point::min ()) {}

      bool
      operator () () noexcept
      {
        time_point now (clock::now ());

        if (last_ == time_point::min () || now - last_ >= interval_)
          return last_ = now, true;

        return false;
      }

      duration interval_;
      time_point last_;
    };

    // Repeated-message suppressor.
    //
    // Returns true only when the format string pointer changes from the
    // previous call.
    //
    struct deduplicate
    {
      bool
      operator () (char const* fmt) noexcept
      {
        if (fmt == last_fmt)
          return false;

        last_fmt = fmt;
        return true;
      }

      char const* last_fmt = nullptr;
    };

    #define IW4X_LOG_SEVERITY(N, L)                                            \
    template <Category C, typename... A>                                       \
    struct N                                                                   \
    {                                                                          \
      N (C,                                                                    \
         char const* f,                                                        \
         A&&... a,                                                             \
         quill::SourceLocation l = quill::SourceLocation::current ())          \
      {                                                                        \
        if constexpr (L >= compiled_minimum_level)                             \
        {                                                                      \
          quill::Logger* q (logger<C> ());                                     \
                                                                               \
          if (q && q->should_log_statement (L))                                \
          {                                                                    \
            quill::log (q, "", L, f, l, static_cast<A&&> (a)...);              \
          }                                                                    \
        }                                                                      \
      }                                                                        \
    };                                                                         \
                                                                               \
    template <Category C, typename... A>                                       \
    N (C, char const*, A&&...) -> N<C, A...>

    IW4X_LOG_SEVERITY (trace_l3, quill::LogLevel::TraceL3);
    IW4X_LOG_SEVERITY (trace_l2, quill::LogLevel::TraceL2);
    IW4X_LOG_SEVERITY (trace_l1, quill::LogLevel::TraceL1);
    IW4X_LOG_SEVERITY (debug,    quill::LogLevel::Debug);
    IW4X_LOG_SEVERITY (info,     quill::LogLevel::Info);
    IW4X_LOG_SEVERITY (notice,   quill::LogLevel::Notice);
    IW4X_LOG_SEVERITY (warning,  quill::LogLevel::Warning);
    IW4X_LOG_SEVERITY (error,    quill::LogLevel::Error);
    IW4X_LOG_SEVERITY (critical, quill::LogLevel::Critical);
    #undef IW4X_LOG_SEVERITY
  }
}
