#pragma once

#include <string>
#include <cstdint>
#include <cstddef>
#include <utility>

#include <libiw4x/gdk/types.hxx>
#include <libiw4x/gdk/vtable.hxx>

namespace iw4x
{
  namespace gdk
  {
    struct binding
    {
      guid                    api;
      guid                    id;
      const interface_object* object;
    };

    static_assert (static_cast<unsigned> (feature::game_streaming) < 32,
                   "a feature family per bit of the set held by a catalog");

    constexpr std::uint32_t
    family_bit (feature f) noexcept
    {
      unsigned n (static_cast<unsigned> (f));

      return n < 32 ? std::uint32_t (1) << n : 0;
    }

    template <published_interface I>
    consteval std::uint32_t
    families_of ()
    {
      std::uint32_t r (0);

      if constexpr (publishes_features<I>)
      {
        for (feature f: I::features)
          r |= family_bit (f);
      }

      return r;
    }

    template <published_interface... I>
    consteval std::string
    collided ()
    {
      constexpr std::size_t n (sizeof... (I));

      const binding bs[n] {{I::api, I::id, nullptr} ...};
      const char*   ns[n] {I::name ...};

      std::string r;

      for (std::size_t i (0); i != n; ++i)
      {
        for (std::size_t j (i + 1); j != n; ++j)
        {
          if (!(bs[i].api == bs[j].api) || !(bs[i].id == bs[j].id))
            continue;

          if (!r.empty ())
            r += ", ";

          r += ns[i];
          r += " and ";
          r += ns[j];
          r += " answer to one pair";
        }
      }

      return r;
    }

    template <published_interface... I>
    struct catalog
    {
      static_assert (sizeof... (I) != 0, "a catalog publishes something");
      static_assert (collided<I...> ().empty (), collided<I...> ());

      static constexpr std::size_t size = sizeof... (I);

      static constexpr binding bindings[size] {{I::api, I::id, &object_of<I>} ...};

      static constexpr std::uint32_t families = (families_of<I> () | ...);

      static const interface_object*
      find (const guid& a, const guid& i) noexcept
      {
#pragma GCC unroll 32
        for (const binding& b: bindings)
        {
          if (b.api == a && b.id == i)
            return b.object;
        }

        return nullptr;
      }

      static bool
      provides (feature f) noexcept
      {
        std::uint32_t b (family_bit (f));

        return b != 0 && (families & b) != 0;
      }
    };

    const interface_object*
    find_interface (const guid& api, const guid& id) noexcept;

    bool
    provides (feature) noexcept;

    std::size_t
    interface_count () noexcept;

    unsigned
    family_count () noexcept;

    void
    announce () noexcept;
  }
}
