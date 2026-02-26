#pragma once

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/core/PatternFormatterOptions.h>
#include <quill/sinks/ConsoleSink.h>
#include <quill/sinks/RotatingFileSink.h>

namespace iw4x
{
  namespace categories
  {
    struct iw4x {};
    struct scheduler {};
    struct detour {};
    struct ui {};
  }

  namespace log
  {
    template <typename C>
    struct policy;

    template <>
    struct policy<categories::iw4x>
    {
      static constexpr string_view     name      = "iw4x";
      static constexpr quill::LogLevel threshold = quill::LogLevel::Info;
    };
    template <>
    struct policy<categories::scheduler>
    {
      static constexpr string_view     name      = "scheduler";
      static constexpr quill::LogLevel threshold = quill::LogLevel::Info;
    };

    template <>
    struct policy<categories::detour>
    {
      static constexpr string_view     name      = "detour";
      static constexpr quill::LogLevel threshold = quill::LogLevel::Info;
    };

    template <>
    struct policy<categories::ui>
    {
      static constexpr string_view     name      = "ui";
      static constexpr quill::LogLevel threshold = quill::LogLevel::Info;
    };

    template <typename C>
    concept Category = requires
    {
      { policy<C>::name      } -> convertible_to<string_view>;
      { policy<C>::threshold } -> convertible_to<quill::LogLevel>;
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
  }
}
