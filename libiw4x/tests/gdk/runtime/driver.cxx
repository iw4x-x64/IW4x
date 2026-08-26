#undef NDEBUG
#include <cassert>

#include <cstring>

#include <windows.h>

#include <libiw4x/gdk/types.hxx>
#include <libiw4x/gdk/async.hxx>
#include <libiw4x/gdk/vtable.hxx>
#include <libiw4x/gdk/runtime.hxx>
#include <libiw4x/gdk/registry.hxx>

using namespace iw4x::gdk;

int
main ()
{
  {
    assert (!initialized ());
  }

  {
    install ();

    HMODULE m (LoadLibraryExA ("xgameruntime.dll",
                               nullptr,
                               LOAD_LIBRARY_SEARCH_SYSTEM32));

    assert (m == image ());

    m = LoadLibraryExA ("C:\\somewhere\\XGameRuntime.DLL",
                        nullptr,
                        LOAD_LIBRARY_SEARCH_SYSTEM32);

    assert (m == image ());

    m = LoadLibraryExA ("kernel32.dll", nullptr, 0);

    assert (m != nullptr && m != image ());

    FreeLibrary (m);

    m = LoadLibraryExA (nullptr, nullptr, 0);

    assert (m == nullptr || m != image ());
  }

  {
    install ();

    assert (LoadLibraryExA ("xgameruntime.dll",
                            nullptr,
                            LOAD_LIBRARY_SEARCH_SYSTEM32) == image ());
  }

  {
    assert (InitializeApiImplEx (0x0000633600002711ULL,
                                 0x0000633600000C6DULL,
                                 0) == S_OK);
    assert (initialized ());

    assert (UninitializeApiImpl () == S_OK);
    assert (!initialized ());

    assert (InitializeApiImpl (25398, 10001) == S_OK);
    assert (initialized ());
  }

  {
    void* o (nullptr);

    assert (QueryApiImpl (&xasync::api, &xasync::id, &o) == S_OK);
    assert (o == const_cast<interface_object*> (&object_of<xasync>));

    guid none {};

    assert (QueryApiImpl (&none, &none, &o) == E_NOINTERFACE);

    assert (QueryApiImpl (nullptr, &xasync::id, &o) == E_POINTER);
    assert (QueryApiImpl (&xasync::api, nullptr, &o) == E_POINTER);
    assert (QueryApiImpl (&xasync::api, &xasync::id, nullptr) == E_POINTER);
  }

  {
    XErrorReport (0x89240107, "a report");
    XErrorReport (0, nullptr);
  }

  {
    auto* p (static_cast<interface_object*> (
               const_cast<void*> (
                 static_cast<const void*> (&object_of<xasync>))));

    auto q (reinterpret_cast<HRESULT (*) (void*, const guid*, void**)> (
              const_cast<void*> (slot_of<xasync> (0x00))));

    void* o (nullptr);

    assert (q (p, nullptr, &o) == S_OK && o == p);
    assert (q (p, nullptr, nullptr) == E_POINTER);

    auto a (reinterpret_cast<unsigned long (*) (void*)> (
              const_cast<void*> (slot_of<xasync> (0x08))));

    auto r (reinterpret_cast<unsigned long (*) (void*)> (
              const_cast<void*> (slot_of<xasync> (0x10))));

    assert (a (p) == 1);
    assert (r (p) == 1);
  }

  {
    assert (UninitializeApiImpl () == S_OK);
    assert (!initialized ());
  }
}
