#include <libiw4x/gdk/runtime.hxx>

#include <atomic>

#include <MinHook.h>

#include <libiw4x/logger.hxx>

#include <libiw4x/gdk/error.hxx>
#include <libiw4x/gdk/registry.hxx>
#include <libiw4x/gdk/task-queue.hxx>

namespace iw4x
{
  namespace gdk
  {
    namespace
    {
      std::atomic<bool> up (false);

      constexpr std::uint64_t expected_version (0x0000633600002711ULL);
      constexpr std::uint64_t expected_api     (0x0000633600000C6DULL);

      constexpr char runtime_module[] = "xgameruntime.dll";

      bool
      is_runtime (LPCSTR n) noexcept
      {
        if (n == nullptr)
          return false;

        const char* f (n);

        for (const char* p (n); *p != '\0'; ++p)
          if (*p == '\\' || *p == '/')
            f = p + 1;

        return _stricmp (f, runtime_module) == 0;
      }

      HMODULE (WINAPI* load_library_ex_a) (LPCSTR, HANDLE, DWORD) (nullptr);

      HMODULE WINAPI
      load_library_ex_a_detour (LPCSTR n, HANDLE f, DWORD d) noexcept
      {
        l3 ("LoadLibraryExA ({}, {:#x})", n != nullptr ? n : "(null)", d);

        if (!is_runtime (n))
          return load_library_ex_a (n, f, d);

        info ("serving {} from this module ({})",
              runtime_module,
              static_cast<const void*> (image ()));

        return image ();
      }

      void
      check (MH_STATUS s, const char* what)
      {
        if (s != MH_OK)
          raise (E_FAIL, "{}: {}", what, MH_StatusToString (s));
      }
    }

    bool
    initialized () noexcept
    {
      return up.load (std::memory_order_acquire);
    }

    void
    install () noexcept
    {
      try
      {
        announce ();

        MH_STATUS s (MH_Initialize ());

        if (s != MH_ERROR_ALREADY_INITIALIZED)
          check (s, "unable to initialize the hook library");

        void* target (nullptr);

        check (MH_CreateHookApiEx (
                 L"kernel32",
                 "LoadLibraryExA",
                 reinterpret_cast<void*> (&load_library_ex_a_detour),
                 reinterpret_cast<void**> (&load_library_ex_a),
                 &target),
               "unable to hook LoadLibraryExA");

        check (MH_EnableHook (target),
               "unable to enable the hook on LoadLibraryExA");
      }
      catch (...)
      {
        text<description_size> m ("unable to install the gaming runtime: "
                                  "{}",
                                  hex (static_cast<std::uint32_t> (
                                         report ("install")),
                                       8));

        fatal (m);
      }
    }

    extern "C"
    {
      HRESULT WINAPI
      InitializeApiImplEx (std::uint64_t v,
                           std::uint64_t a,
                           std::uint32_t f) noexcept
      {
        return guard ("InitializeApiImplEx", [&] () -> HRESULT
        {
          if (v != expected_version || a != expected_api)
            warn ("unrecovered runtime version ({:#x}, {:#x}), expected "
                  "({:#x}, {:#x})",
                  v, a, expected_version, expected_api);

          info ("gaming runtime up, flags {:#x}", f);

          up.store (true, std::memory_order_release);
          return S_OK;
        });
      }

      HRESULT WINAPI
      InitializeApiImpl (std::uint64_t v, std::uint64_t a) noexcept
      {
        return InitializeApiImplEx ((v << 32) | a, (v << 32) | 0xC6D, 0);
      }

      HRESULT WINAPI
      QueryApiImpl (const guid* a, const guid* i, void** out) noexcept
      {
        return guard ("QueryApiImpl", [&] () -> HRESULT
        {
          if (a == nullptr || i == nullptr || out == nullptr)
            return E_POINTER;

          if (const interface_object* o = find_interface (*a, *i))
          {
            l2 ("QueryApiImpl -> {}", o->name);

            *out = const_cast<interface_object*> (o);
            return S_OK;
          }

          warn ("no interface for api {:08X}-{:04X}-{:04X} / "
                "{:08X}-{:04X}-{:04X}",
                a->data1, a->data2, a->data3,
                i->data1, i->data2, i->data3);

          return E_NOINTERFACE;
        });
      }

      void WINAPI
      XErrorReport (std::uint32_t c, const char* m) noexcept
      {
        guard ("XErrorReport", [&] () -> void
        {
          warn ("the runtime shim reported {:#010x}: {}",
                c,
                m != nullptr ? m : "");
        });
      }

      HRESULT WINAPI
      UninitializeApiImpl () noexcept
      {
        return guard ("UninitializeApiImpl", [&] () -> HRESULT
        {
          info ("gaming runtime down");

          up.store (false, std::memory_order_release);

          stop_queues ();

          return S_OK;
        });
      }
    }
  }
}
