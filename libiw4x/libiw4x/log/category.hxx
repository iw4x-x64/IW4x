#pragma once

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/core/PatternFormatterOptions.h>
#include <quill/sinks/ConsoleSink.h>
#include <quill/sinks/RotatingFileSink.h>

namespace iw4x
{
  // Tag types used to identify distinct logging categories. These are used to
  // statically dispatch to the corresponding policy and logger instance
  // without passing instances around.
  //
  namespace categories
  {
    struct iw4x {};
  }

  namespace log
  {
    // Category policy. Leave the primary template undefined so that failing to
    // provide a policy for a new category results in a clear compile-time error
    // rather than some silent fallback.
    //
    template <typename Category>
    struct category_policy;

    // Policy for the default category. Note that we map this to the Info
    // threshold to avoid spamming the console with debug traces.
    //
    template <>
    struct category_policy<categories::iw4x>
    {
      static constexpr string_view     name      = "iw4x";
      static constexpr quill::LogLevel threshold = quill::LogLevel::Info;
    };

    // Sanity check for category policies. Expect them to provide at least a
    // name string and a minimum log level that can be fed to Quill during the
    // logger registration phase.
    //
    template <typename T>
    concept log_category = requires
    {
      { category_policy<T>::name      } -> convertible_to<string_view>;
      { category_policy<T>::threshold } -> convertible_to<quill::LogLevel>;
    };

    namespace detail
    {
      // Storage for the category-specific logger instance. Returns a reference
      // to the static pointer so the logging initialization routine can inject
      // the actual Quill logger once it is created.
      //
      template <typename Category> quill::Logger*&
      category_logger_ref () noexcept
      {
        static quill::Logger* p (nullptr);
        return p;
      }
    }

    // Fast-path accessor for the logger pointer. Use this directly in the
    // logging macros to bypass Quill's internal map lookups on every log
    // invocation. Note that the logging subsystem must have already populated
    // this pointer before the first log call is made.
    //
    template <typename Category> quill::Logger*
    category_logger () noexcept
    {
      return detail::category_logger_ref<Category> ();
    }
  }
}
