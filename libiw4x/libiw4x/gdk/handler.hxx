#pragma once

#include <cstdint>
#include <cstddef>
#include <concepts>
#include <type_traits>

#include <libiw4x/gdk/sync.hxx>

namespace iw4x
{
  namespace gdk
  {
    template <typename H>
    concept registered_handler =
      std::is_object_v<H> && std::is_trivially_copyable_v<H>;

    template <registered_handler H, std::size_t N = 8>
    class handler_table
    {
    public:
      static constexpr std::size_t capacity = N;

      handler_table () noexcept = default;

      handler_table (const handler_table&) = delete;
      handler_table& operator= (const handler_table&) = delete;

      std::uint64_t
      add (const H& h) noexcept
      {
        scope_lock l (mutex_);

        for (std::size_t i (0); i != N; ++i)
        {
          if (used_[i])
            continue;

          handlers_[i] = h;
          used_[i] = true;

          return static_cast<std::uint64_t> (i) + 1;
        }

        return 0;
      }

      bool
      remove (std::uint64_t token) noexcept
      {
        scope_lock l (mutex_);

        if (token == 0 || token > N)
          return false;

        bool& u (used_[token - 1]);

        if (!u)
          return false;

        u = false;
        return true;
      }

      std::size_t
      live (H* out, std::size_t n) const noexcept
      {
        scope_lock l (mutex_);

        std::size_t k (0);

        for (std::size_t i (0); i != N && k != n; ++i)
        {
          if (used_[i])
            out[k++] = handlers_[i];
        }

        return k;
      }

      std::size_t
      size () const noexcept
      {
        scope_lock l (mutex_);

        std::size_t k (0);

        for (std::size_t i (0); i != N; ++i)
          k += used_[i] ? 1 : 0;

        return k;
      }

    private:
      mutable mutex mutex_;

      H    handlers_[N] {};
      bool used_[N] {};
    };
  }
}
