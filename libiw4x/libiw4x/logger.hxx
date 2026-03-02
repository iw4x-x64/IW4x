#pragma once

#include <concepts>
#include <string_view>

#include <quill/Backend.h>
#include <quill/LogFunctions.h>

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

  LIBIW4X_SYMEXPORT extern logger* logger;

  namespace log
  {
    namespace categories
    {
      struct iw4x {};
    }

    template <typename C>
    struct policy;

    template <>
    struct policy<categories::iw4x>
    {
      static constexpr std::string_view     name      = "iw4x";
      static constexpr quill::LogLevel threshold = quill::LogLevel::Info;
    };

    template <typename C>
    concept Category = requires
    {
      { policy<C>::name      } -> std::convertible_to<std::string_view>;
      { policy<C>::threshold } -> std::convertible_to<quill::LogLevel>;
    };

    namespace detail
    {
      template <typename C>
      quill::Logger*&
      logger () noexcept
      {
        static quill::Logger* p (nullptr);
        return p;
      }
    }

    template <typename C>
    quill::Logger*
    logger () noexcept
    {
      return detail::logger<C> ();
    }

    // Minimum severity level.
    //
    // The idea here is that every log statement whose severity falls strictly
    // below this threshold is to be removed by the compiler entirely. That is,
    // the threshold is a compile-time constant so the optimizer can fold the
    // guarding if constexpr in each dispatch struct to a no-op.
    //
    // Note that development builds open the full trace range.
    //
  #if LIBIW4X_DEVELOP
    inline constexpr quill::LogLevel
      compiled_minimum_level (quill::LogLevel::TraceL3);
#else
    inline constexpr quill::LogLevel
      compiled_minimum_level (quill::LogLevel::Info);
#endif

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
  }
}
