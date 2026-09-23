#include <libiw4x/gdk/vtable.hxx>

#include <libiw4x/logger.hxx>

using namespace std;

namespace iw4x
{
  namespace gdk
  {
    namespace
    {
      const char*
      interface_name (void* self) noexcept
      {
        if (self == nullptr)
          return "<no interface>";

        const char* n (static_cast<const interface_object*> (self)->name);

        return n != nullptr ? n : "<unnamed interface>";
      }
    }

    HRESULT WINAPI
    query_interface (void* self, const guid*, void** out) noexcept
    {
      if (out == nullptr)
        return E_POINTER;

      *out = self;
      return S_OK;
    }

    unsigned long WINAPI
    add_ref (void*) noexcept
    {
      return 1;
    }

    unsigned long WINAPI
    release (void*) noexcept
    {
      return 1;
    }

    HRESULT
    report_unrecovered (void* self, size_t o) noexcept
    {
      warn ("{}: call into unrecovered slot {:#x}", interface_name (self), o);

      return E_NOTIMPL;
    }

    HRESULT
    report_declined (void* self, size_t o) noexcept
    {
      l1 ("{}: slot {:#x} is not provided", interface_name (self), o);

      return E_NOTIMPL;
    }

    char
    report_declined_false (void* self, size_t o) noexcept
    {
      l1 ("{}: slot {:#x} is not provided, answering false",
          interface_name (self),
          o);

      return 0;
    }
  }
}
