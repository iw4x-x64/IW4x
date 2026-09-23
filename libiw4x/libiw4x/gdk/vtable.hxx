#pragma once

#include <meta>
#include <string>
#include <vector>
#include <cstddef>
#include <utility>
#include <concepts>

#include <libiw4x/hexadecimal.hxx>

#include <libiw4x/gdk/types.hxx>

namespace iw4x
{
  namespace gdk
  {
    struct slot
    {
      std::size_t offset;
    };

    inline constexpr std::size_t max_slot (0x200);
    inline constexpr std::size_t slot_count (max_slot / sizeof (void*));
    inline constexpr std::size_t first_slot (3 * sizeof (void*));

    struct interface_object
    {
      const void* const* table;
      const char*        name;
    };

    static_assert (offsetof (interface_object, table) == 0,
                   "the interface object begins with its table pointer");

    template <typename I>
    concept published_interface = requires
    {
      { I::label } -> std::convertible_to<const char*>;
      { I::api }   -> std::convertible_to<const guid&>;
      { I::id }    -> std::convertible_to<const guid&>;
    };

    template <typename I>
    concept publishes_features = requires
    {
      { I::features };
    };

    template <typename I>
    concept declines_slots = requires
    {
      { I::declined };
    };

    template <typename I>
    concept declines_slots_false = requires
    {
      { I::declined_false };
    };

    HRESULT WINAPI
    query_interface (void* self, const guid*, void** out) noexcept;

    unsigned long WINAPI
    add_ref (void*) noexcept;

    unsigned long WINAPI
    release (void*) noexcept;

    HRESULT
    report_unrecovered (void* self, std::size_t offset) noexcept;

    HRESULT
    report_declined (void* self, std::size_t offset) noexcept;

    char
    report_declined_false (void* self, std::size_t offset) noexcept;

    template <std::size_t O>
    HRESULT WINAPI
    unrecovered (void* self, ...) noexcept
    {
      return report_unrecovered (self, O);
    }

    template <std::size_t O>
    HRESULT WINAPI
    declining (void* self, ...) noexcept
    {
      return report_declined (self, O);
    }

    template <std::size_t O>
    char WINAPI
    declining_false (void* self, ...) noexcept
    {
      return report_declined_false (self, O);
    }

    template <published_interface I>
    consteval bool
    is_declined (std::size_t o)
    {
      if constexpr (declines_slots<I>)
      {
        for (std::size_t d: I::declined)
          if (d == o)
            return true;
      }

      return false;
    }

    template <published_interface I>
    consteval bool
    is_declined_false (std::size_t o)
    {
      if constexpr (declines_slots_false<I>)
      {
        for (std::size_t d: I::declined_false)
          if (d == o)
            return true;
      }

      return false;
    }

    template <published_interface I>
    consteval std::meta::info
    claimant (std::size_t o)
    {
      for (std::meta::info m: std::meta::members_of (
             ^^I, std::meta::access_context::current ()))
      {
        for (std::meta::info a:
               std::meta::annotations_of_with_type (m, ^^slot))
        {
          if (std::meta::extract<slot> (a).offset == o)
            return m;
        }
      }

      return std::meta::info ();
    }

    template <published_interface I, std::size_t O>
    consteval std::meta::info
    entry_of ()
    {
      constexpr std::size_t o (O * sizeof (void*));

      if constexpr (o == 0)
        return ^^query_interface;
      else if constexpr (o == sizeof (void*))
        return ^^add_ref;
      else if constexpr (o == 2 * sizeof (void*))
        return ^^release;
      else
      {
        constexpr std::meta::info m (claimant<I> (o));

        if constexpr (m != std::meta::info ())
          return m;
        else if constexpr (is_declined<I> (o))
          return ^^declining<o>;
        else if constexpr (is_declined_false<I> (o))
          return ^^declining_false<o>;
        else
          return ^^unrecovered<o>;
      }
    }

    template <published_interface I>
    consteval std::string
    malformed ()
    {
      std::string r;

      auto complain ([&r] (std::size_t o, const char* what)
      {
        if (!r.empty ())
          r += ", ";

        char h[2 * sizeof (std::size_t)];

        const hex_to_chars_result x (
          to_hex_chars_minimal (h, h + sizeof (h), o, hex_case::upper));

        r += I::label;
        r += ": slot 0x";
        r.append (h, x.ptr);
        r += ' ';
        r += what;
      });

      auto placed ([&complain] (std::size_t o) -> bool
      {
        if (o % sizeof (void*) != 0)
        {
          complain (o, "is not a whole number of words");
          return false;
        }

        if (o < first_slot)
        {
          complain (o, "lands on IUnknown");
          return false;
        }

        if (o >= max_slot)
        {
          complain (o, "is past the end of the table");
          return false;
        }

        return true;
      });

      std::vector<std::size_t> claimed;

      for (std::meta::info m: std::meta::members_of (
             ^^I, std::meta::access_context::current ()))
      {
        auto as (std::meta::annotations_of_with_type (m, ^^slot));

        if (as.empty ())
          continue;

        if (!std::meta::is_function (m) || !std::meta::is_static_member (m))
        {
          if (!r.empty ())
            r += ", ";

          r += I::label;
          r += ": a slot is claimed by something that is not a static member "
               "function";
          continue;
        }

        for (std::meta::info a: as)
        {
          std::size_t o (std::meta::extract<slot> (a).offset);

          if (!placed (o))
            continue;

          bool twice (false);

          for (std::size_t c: claimed)
            twice = twice || c == o;

          if (twice)
            complain (o, "is claimed twice");
          else
            claimed.push_back (o);

          if (is_declined<I> (o) || is_declined_false<I> (o))
            complain (o, "is both implemented and declined");
        }
      }

      if constexpr (declines_slots<I>)
      {
        for (std::size_t o: I::declined)
          placed (o);
      }

      if constexpr (declines_slots_false<I>)
      {
        for (std::size_t o: I::declined_false)
        {
          if (placed (o) && is_declined<I> (o))
            complain (o, "is declined twice");
        }
      }

      return r;
    }

    template <published_interface I, typename S>
    struct interface_table;

    template <published_interface I, std::size_t... O>
    struct interface_table<I, std::index_sequence<O...>>
    {
      static_assert (malformed<I> ().empty (), malformed<I> ());

      static const void* const entries[sizeof... (O)];
    };

    template <published_interface I, std::size_t... O>
    const void* const
    interface_table<I, std::index_sequence<O...>>::entries[sizeof... (O)] =
    {
      reinterpret_cast<const void*> (&[: entry_of<I, O> () :]) ...
    };

    template <published_interface I>
    using table_of = interface_table<I, std::make_index_sequence<slot_count>>;

    template <published_interface I>
    inline const interface_object object_of
    {
      table_of<I>::entries,
      I::label
    };

    template <published_interface I>
    inline const void*
    slot_of (std::size_t o) noexcept
    {
      return o < max_slot && o % sizeof (void*) == 0
               ? table_of<I>::entries[o / sizeof (void*)]
               : nullptr;
    }
  }
}
