#pragma once

#include <cstddef>
#include <concepts>
#include <type_traits>

#include <libiw4x/gdk/text.hxx>
#include <libiw4x/gdk/error.hxx>

namespace iw4x
{
  namespace gdk
  {
    template <typename T>
    concept out_parameter = std::is_object_v<T> && !std::is_const_v<T>;

    template <out_parameter T>
    inline T&
    answer (T* p)
    {
      if (p == nullptr)
        raise (E_POINTER, "no result pointer");

      return *p;
    }

    void
    copy_out (chars value, std::size_t size, char* buffer, std::size_t* used);
  }
}
