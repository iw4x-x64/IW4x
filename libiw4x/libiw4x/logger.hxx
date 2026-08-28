#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace iw4x
{
  namespace logger
  {
    namespace detail
    {
      enum class level: std::uint8_t
      {
        trace3  = 0,
        trace2  = 1,
        trace1  = 2,
        info    = 4,
        warning = 6,
        error   = 7
      };

      struct location
      {
        const char* file;
        const char* function;
        std::uint_least32_t line;

        static constexpr location
        current (const char* file = __builtin_FILE (),
                 const char* function = __builtin_FUNCTION (),
                 std::uint_least32_t line = __builtin_LINE ()) noexcept
        {
          return location {file, function, line};
        }
      };

      struct argument
      {
        enum class kind: std::uint8_t
        {
          signed_integer,
          unsigned_integer,
          real,
          boolean,
          character,
          string,
          pointer
        };

        static constexpr std::size_t unmeasured =
          static_cast<std::size_t> (-1);

        static constexpr std::size_t max_count = 8;

        kind which;

        union
        {
          long long          signed_integer;
          unsigned long long unsigned_integer;
          double             real;
          bool               boolean;
          char               character;
          const char*        string;
          const void*        pointer;
        };

        std::size_t size;
      };

      template <typename T>
      constexpr argument
      make (T&& v) noexcept
      {
        using U = std::remove_cvref_t<T>;
        using kind = argument::kind;

        if constexpr (std::is_same_v<U, bool>)
          return {kind::boolean, {.boolean = v}, 0};
        else if constexpr (std::is_same_v<U, char>)
          return {kind::character, {.character = v}, 0};
        else if constexpr (std::is_floating_point_v<U>)
          return {kind::real, {.real = static_cast<double> (v)}, 0};
        else if constexpr (std::is_enum_v<U>)
          return {kind::signed_integer,
                  {.signed_integer = static_cast<long long> (
                     static_cast<std::underlying_type_t<U>> (v))},
                  0};
        else if constexpr (std::is_integral_v<U> && std::is_signed_v<U>)
          return {kind::signed_integer, {.signed_integer = v}, 0};
        else if constexpr (std::is_integral_v<U>)
          return {kind::unsigned_integer, {.unsigned_integer = v}, 0};
        else if constexpr (std::is_convertible_v<U, const char*>)
          return {kind::string, {.string = v}, argument::unmeasured};
        else if constexpr (requires { v.data (); v.size (); })
          return {kind::string, {.string = v.data ()}, v.size ()};
        else
          return {kind::pointer, {.pointer = v}, 0};
      }

      void
      log (level,
           const location&,
           const char* format,
           const argument*,
           std::size_t) noexcept;

      template <level L, typename... A>
      struct statement
      {
        statement (
          const char* format,
          A&&... args,
          location where = location::current ())
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
          // public statements differ only in the level. In particular,
          // keeping the forwarding here is important since otherwise
          // every new level would have to reproduce this constructor
          // exactly.
          //
          // Note also the slightly subtle source location business. We
          // want the location of
          //
          //   info ("...");
          //
          // and not the location of log() below. For that to work the
          // default argument has to be evaluated while constructing the
          // public statement and then carried with us into this common
          // implementation. Moving location::current() into the body
          // would therefore quietly make every message point here, which
          // is generally the opposite of what somebody looking at a log
          // wants.
          //
          static_assert (sizeof... (A) <= argument::max_count,
                         "too many arguments in a logging statement");

          if constexpr (sizeof... (A) != 0)
          {
            const argument a[] {make (std::forward<A> (args))...};
            log (L, where, format, a, sizeof... (A));
          }
          else
            log (L, where, format, nullptr, 0);
        }
      };
    }

    void
    stop () noexcept;
  }

  // Note that we need the deduction guide even though the constructor
  // itself is inherited. The arguments belong to the class template and
  // have to be known before that constructor can be selected.

  template <typename... A>
  struct fail: logger::detail::statement<logger::detail::level::error, A...>
  {
    using logger::detail::statement<logger::detail::level::error,
                                    A...>::statement;
  };

  template <typename... A>
  fail (const char*, A&&...) -> fail<A...>;

  template <typename... A>
  struct warn: logger::detail::statement<logger::detail::level::warning, A...>
  {
    using logger::detail::statement<logger::detail::level::warning,
                                    A...>::statement;
  };

  template <typename... A>
  warn (const char*, A&&...) -> warn<A...>;

  template <typename... A>
  struct info: logger::detail::statement<logger::detail::level::info, A...>
  {
    using logger::detail::statement<logger::detail::level::info,
                                    A...>::statement;
  };

  template <typename... A>
  info (const char*, A&&...) -> info<A...>;

  template <typename... A>
  struct l1: logger::detail::statement<logger::detail::level::trace1, A...>
  {
    using logger::detail::statement<logger::detail::level::trace1,
                                    A...>::statement;
  };

  template <typename... A>
  l1 (const char*, A&&...) -> l1<A...>;

  template <typename... A>
  struct l2: logger::detail::statement<logger::detail::level::trace2, A...>
  {
    using logger::detail::statement<logger::detail::level::trace2,
                                    A...>::statement;
  };

  template <typename... A>
  l2 (const char*, A&&...) -> l2<A...>;

  template <typename... A>
  struct l3: logger::detail::statement<logger::detail::level::trace3, A...>
  {
    using logger::detail::statement<logger::detail::level::trace3,
                                    A...>::statement;
  };

  template <typename... A>
  l3 (const char*, A&&...) -> l3<A...>;
}
