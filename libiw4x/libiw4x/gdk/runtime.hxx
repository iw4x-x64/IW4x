#pragma once

#include <cstdint>

#include <libiw4x/export.hxx>

#include <libiw4x/gdk/types.hxx>

namespace iw4x
{
  namespace gdk
  {
    extern "C"
    {
      LIBIW4X_SYMEXPORT HRESULT WINAPI
      QueryApiImpl (const guid*, const guid*, void**) noexcept;

      LIBIW4X_SYMEXPORT HRESULT WINAPI
      InitializeApiImpl (std::uint64_t version, std::uint64_t api) noexcept;

      LIBIW4X_SYMEXPORT HRESULT WINAPI
      InitializeApiImplEx (std::uint64_t version,
                           std::uint64_t api,
                           std::uint32_t flags) noexcept;

      LIBIW4X_SYMEXPORT void WINAPI
      XErrorReport (std::uint32_t code, const char* message) noexcept;

      LIBIW4X_SYMEXPORT HRESULT WINAPI
      UninitializeApiImpl () noexcept;
    }

    void
    install () noexcept;

    bool
    initialized () noexcept;
  }
}
