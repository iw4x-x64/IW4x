#include <libiw4x/r/backend/r-d3d9.hxx>

#include <vector>
#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <functional>

#include <d3d9.h>
#include <psapi.h>

#pragma comment(lib, "d3d9.lib")

using namespace std;

namespace iw4x
{
  namespace r
  {
    namespace
    {
      // Callbacks.
      //
      vector<function<void (IDirect3DDevice9*)>> cbs_f; // Frame.
      vector<function<void ()>>                  cbs_l; // Lost.
      vector<function<void ()>>                  cbs_r; // Reset.

      // Signatures.
      //
      using end_fn = HRESULT (APIENTRY*) (IDirect3DDevice9*);
      using rst_fn = HRESULT (APIENTRY*) (IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
      using crt_fn = HRESULT (APIENTRY*) (IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);

      // Trampolines.
      //
      end_fn o_end (nullptr);
      rst_fn o_rst (nullptr);
      crt_fn o_crt (nullptr);

      // VTable patching.
      //
      // The vtable is usually resident in the .rdata section. If we try to
      // write to it directly, we will trigger an access violation. We have
      // to lift the write protection, swap the pointer, and put the
      // protection back.
      //
      void
      patch (void** vt, int i, void* f)
      {
        DWORD p;
        void* a (&vt[i]);

        if (!VirtualProtect (a, sizeof (void*), PAGE_EXECUTE_READWRITE, &p))
        {
          cerr << "error: unable to unprotect vtable at " << i;
          exit (1);
        }

        vt[i] = f;

        if (!VirtualProtect (a, sizeof (void*), p, &p))
        {
          // Unlike iw4mp.exe, we don't want to leave d3d9 vtable writable as
          // we might mask memory corruption bugs later on.
          //
          cerr << "warn: unable to re-protect vtable at " << i;
        }
      }

      // Hook: EndScene (42).
      //
      // We use this as our "frame" trigger. It's the standard location to
      // draw overlays because the back buffer is still accessible but the
      // game has finished its geometry submission.
      //
      HRESULT APIENTRY
      h_end (IDirect3DDevice9* d)
      {
        if (!o_end) throw logic_error ("end_scene called without original");

        try
        {
          for (auto& c : cbs_f) c (d);
        }
        catch (...)
        {
          cerr << "error: exception in frame callback";
          exit (1);
        }

        return o_end (d);
      }

      // Hook: Reset (16).
      //
      // Handling the device lifecycle is tricky. D3D9 requires us to
      // Release() all default pool resources before calling Reset(), and
      // recreate them after. If we don't, Reset() fails.
      //
      HRESULT APIENTRY
      h_rst (IDirect3DDevice9* d, D3DPRESENT_PARAMETERS* pp)
      {
        if (!o_rst) throw logic_error ("reset called without original");

        // Phase 1: Device is lost. Release resources.
        //
        try
        {
          for (auto& c : cbs_l) c ();
        }
        catch (...)
        {
          cerr << "error: exception in lost callback";
          exit (1);
        }

        HRESULT r (o_rst (d, pp));

        if (SUCCEEDED (r))
        {
          // Phase 2: Device is back. Recreate resources.
          //
          try
          {
            for (auto& c : cbs_r) c ();
          }
          catch (...)
          {
            cerr << "error: exception in reset callback";
            exit (1);
          }

          void** vt (*reinterpret_cast<void***> (d));

          // External overlays (Steam, Discord, etc.) often re-hook the
          // vtable immediately after a Reset, overwriting our EndScene
          // hook. We have to check if we are still the owner; if not,
          // force our way back in.
          //
          if (vt[42] != h_end)
          {
            if (!o_end) o_end = reinterpret_cast<end_fn> (vt[42]);
            patch (vt, 42, reinterpret_cast<void*> (h_end));
          }
        }
        else
        {
          // If the driver rejects the parameters, there is nothing we can
          // do.
          //
          cerr << "error: reset failed: " << (void*) (uintptr_t) r;
          exit (1);
        }

        return r;
      }

      // Hook: CreateDevice (16 on IDirect3D9).
      //
      // This is the trap. We hook the creation call so we can grab the
      // resulting device pointer immediately after the game initializes
      // the renderer.
      //
      HRESULT APIENTRY
      h_crt (IDirect3D9* i, UINT a, D3DDEVTYPE t, HWND w, DWORD f, D3DPRESENT_PARAMETERS* pp, IDirect3DDevice9** out)
      {
        HRESULT r (o_crt (i, a, t, w, f, pp, out));

        if (SUCCEEDED (r) && out && *out)
        {
          IDirect3DDevice9* d (*out);
          void** vt (*reinterpret_cast<void***> (d));

          // Capture the original function pointers so we can trampoline
          // back to the driver.
          //
          if (!o_rst) o_rst = reinterpret_cast<rst_fn> (vt[16]);
          if (!o_end) o_end = reinterpret_cast<end_fn> (vt[42]);

          // Install our hooks into the active device.
          //
          patch (vt, 16, reinterpret_cast<void*> (h_rst));
          patch (vt, 42, reinterpret_cast<void*> (h_end));
        }
        else
        {
          cerr << "error: create_device failed: " << (void*) (uintptr_t) r;
          exit (1);
        }

        return r;
      }
    }

    void
    on_frame (function<void (IDirect3DDevice9*)> cb)
    {
      cbs_f.push_back (move (cb));
    }

    void
    on_reset (function<void ()> l, function<void ()> r)
    {
      cbs_l.push_back (move (l));
      cbs_r.push_back (move (r));
    }

    void
    create ()
    {
      // We need the module handle to locate the export.
      //
      HMODULE hm (GetModuleHandle ("d3d9"));
      if (!hm)
      {
        cerr << "error: d3d9 module missing";
        exit (1);
      }

      FARPROC fp (GetProcAddress (hm, "Direct3DCreate9"));
      if (!fp)
      {
        cerr << "error: Direct3DCreate9 export missing";
        exit (1);
      }

      using d3d_crt_fn = IDirect3D9*(WINAPI*) (UINT);
      IDirect3D9* d (reinterpret_cast<d3d_crt_fn> (fp) (D3D_SDK_VERSION));

      if (!d)
      {
        cerr << "error: dummy d3d9 interface creation failed";
        exit (1);
      }

      void** vt (*reinterpret_cast<void***> (d));

      if (!o_crt)
      {
        o_crt = reinterpret_cast<crt_fn> (vt[16]);
      }

      // We hook the dummy interface's CreateDevice. Since all interfaces
      // created by this DLL share the same vtable logic, this hook will
      // intercept the game's subsequent creation call.
      //
      patch (vt, 16, reinterpret_cast<void*> (h_crt));

      d->Release ();
    }
  }
}
