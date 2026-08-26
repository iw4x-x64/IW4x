#pragma once

#include <utility>
#include <concepts>
#include <type_traits>

namespace iw4x
{
  namespace gdk
  {
    template <typename T>
    concept owned = std::is_object_v<T> && std::has_virtual_destructor_v<T>;

    template <owned T>
    class owner
    {
    public:
      owner () noexcept = default;

      explicit
      owner (T* p) noexcept: p_ (p) {}

      owner (owner&& o) noexcept: p_ (o.p_)
      {
        o.p_ = nullptr;
      }

      template <owned D>
      requires std::derived_from<D, T>
      owner (owner<D>&& o) noexcept: p_ (o.release ()) {}

      template <owned D>
      requires std::derived_from<D, T>
      owner&
      operator= (owner<D>&& o) noexcept
      {
        reset ();

        p_ = o.release ();

        return *this;
      }

      owner&
      operator= (owner&& o) noexcept
      {
        if (this != &o)
        {
          reset ();

          p_ = o.p_;
          o.p_ = nullptr;
        }

        return *this;
      }

      owner (const owner&) = delete;
      owner& operator= (const owner&) = delete;

      ~owner ()
      {
        reset ();
      }

      void
      reset () noexcept
      {
        delete p_;
        p_ = nullptr;
      }

      T*
      release () noexcept
      {
        T* r (p_);
        p_ = nullptr;

        return r;
      }

      T*
      get () const noexcept
      {
        return p_;
      }

      T*
      operator-> () const noexcept
      {
        return p_;
      }

      T&
      operator* () const noexcept
      {
        return *p_;
      }

      explicit
      operator bool () const noexcept
      {
        return p_ != nullptr;
      }

    private:
      T* p_ = nullptr;
    };

    template <owned T, typename... A>
    requires std::constructible_from<T, A...>
    inline owner<T>
    make (A&&... a)
    {
      return owner<T> (new T (std::forward<A> (a)...));
    }
  }
}
