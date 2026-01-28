#include <string>
#include <iostream>
#include <stdexcept>

#include <libiw4x/r/overlay/r-imgui.hxx>
#include <libiw4x/r/backend/r-d3d9.hxx>

#include <libimgui/imgui.h>
#include <libimgui/backends/imgui_impl_dx9.h>
#include <libimgui/backends/imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT
ImGui_ImplWin32_WndProcHandler (HWND, UINT, WPARAM, LPARAM);

namespace iw4x
{
  namespace r
  {
    namespace
    {
      // We rely on static storage here rather than a context object passed
      // around. Since we are injecting into an existing game process, strictly
      // speaking, there is only ever one "device" and one "window" relevant
      // to the rendering thread. If we ever need to support multiple swap
      // chains, we will have to wrap this.
      //
      IDirect3DDevice9* d_ (nullptr);
      HWND              w_ (nullptr);
      WNDPROC           p_ (nullptr);

      // rst_: Tracks if we've registered the reset callbacks. We only want
      // to do this once, regardless of how many times the window handle
      // changes.
      //
      // skip_: We skip the first frame after hooking. The message pump often
      // needs a cycle to settle after we inject the subclass, otherwise we
      // risk processing input messages with an invalid state or causing
      // ghost inputs in the game.
      //
      bool rst_  (false);
      bool skip_ (false);

      struct state
      {
        IDirect3DStateBlock9* b;

        // ImGui is invasive; it alters D3D9 render state (blending, stencils,
        // etc.). The game engine expects the state to be preserved across
        // calls. We capture the state block before ImGui runs and apply it
        // immediately after.
        //
        explicit
        state (IDirect3DDevice9* d)
            : b (nullptr)
        {
          // If we fail to create a block (e.g. device lost), we just bail.
          //
          if (FAILED (d->CreateStateBlock (D3DSBT_ALL, &b)))
            b = nullptr;
          else
            b->Capture ();
        }

        ~state ()
        {
          if (b)
          {
            b->Apply ();
            b->Release ();
          }
        }

        explicit operator bool () const { return b != nullptr; }
      };

      LRESULT CALLBACK
      subclass (HWND h, UINT m, WPARAM w, LPARAM l)
      {
        // If the context was destroyed but the hook remains
        // (which implies a teardown race condition), we must bail out to
        // avoid accessing freed memory.
        //
        if (ImGui::GetCurrentContext ())
        {
          if (ImGui_ImplWin32_WndProcHandler (h, m, w, l))
            return true;

          ImGuiIO& io (ImGui::GetIO ());

          // We only eat the input if ImGui explicitly requests capture.
          // Otherwise, we want the user to be able to shoot/move even
          // if the menu is visible (but not focused).
          //
          if (io.WantCaptureMouse || io.WantCaptureKeyboard)
          {
            switch (m)
            {
              case WM_LBUTTONDOWN: case WM_LBUTTONUP:   case WM_MOUSEMOVE:
              case WM_MOUSEWHEEL:  case WM_MOUSEHWHEEL: case WM_RBUTTONDOWN:
              case WM_RBUTTONUP:   case WM_MBUTTONDOWN: case WM_MBUTTONUP:
              case WM_KEYDOWN:     case WM_KEYUP:       case WM_CHAR:
                return 0;
            }
          }
        }

        if (!p_)
          throw std::logic_error ("subclass invoked without proc");

        // Forward to the original procedure. We need to respect the character
        // set of the window to avoid mangling text input.
        //
        return IsWindowUnicode (h)
             ? CallWindowProcW (p_, h, m, w, l)
             : CallWindowProcA (p_, h, m, w, l);
      }

      void
      free ()
      {
        if (d_)
        {
          // We must be careful to restore the window procedure *before*
          // destroying the ImGui context. If we destroy the context first,
          // the OS might trigger a callback into our hook (e.g., WM_DESTROY),
          // which would then try to access the dead ImGui context.
          //
          if (w_ && p_ && IsWindow (w_))
          {
            SetLastError (0);
            if (!SetWindowLongPtrW (w_,
                                    GWLP_WNDPROC,
                                    reinterpret_cast<LONG_PTR> (p_)))
            {
              if (GetLastError () != 0)
                std::cerr << "error: failed to restore wndproc" << std::endl;
            }
          }

          w_ = nullptr;
          p_ = nullptr;

          ImGui_ImplDX9_Shutdown ();
          ImGui_ImplWin32_Shutdown ();

          if (ImGui::GetCurrentContext ())
            ImGui::DestroyContext ();

          d_ = nullptr;
        }
      }

      HWND
      find_wnd (IDirect3DDevice9* d)
      {
        // The D3D device doesn't always explicitly own the window we want to
        // draw on. We attempt to extract the focus window first (which is
        // usually correct for full screen games), falling back to the swap
        // chain's presentation parameters if that fails.
        //
        D3DDEVICE_CREATION_PARAMETERS cp;
        if (SUCCEEDED (d->GetCreationParameters (&cp)))
        {
          if (IsWindow (cp.hFocusWindow))
            return cp.hFocusWindow;
        }

        IDirect3DSwapChain9* sc (nullptr);
        if (SUCCEEDED (d->GetSwapChain (0, &sc)))
        {
          D3DPRESENT_PARAMETERS pp;
          if (SUCCEEDED (sc->GetPresentParameters (&pp)))
          {
            sc->Release ();
            if (IsWindow (pp.hDeviceWindow))
              return pp.hDeviceWindow;
          }
          sc->Release ();
        }
        return nullptr;
      }

      void
      init (IDirect3DDevice9* d)
      {
        // We perform a full teardown before setup. This handles cases where
        // the device pointer changes so that we we don't leak the old context.
        //
        free ();

        IMGUI_CHECKVERSION ();
        if (!ImGui::CreateContext ())
        {
          std::cerr << "error: unable to create context" << std::endl;
          return;
        }

        ImGuiIO& io (ImGui::GetIO ());
        io.MouseDrawCursor = true;
        // We manage config manually; don't save to disk or we'll litter the
        // game directory.
        io.IniFilename = nullptr;

        HWND h (find_wnd (d));
        if (!h)
        {
          std::cerr << "error: no device window" << std::endl;
          ImGui::DestroyContext ();
          return;
        }

        if (!ImGui_ImplWin32_Init (h) || !ImGui_ImplDX9_Init (d))
        {
          std::cerr << "error: unable to init backend" << std::endl;
          ImGui::DestroyContext ();
          return;
        }

        // Note that we cast to WNDPROC here because SetWindowLongPtr can
        // return a handle to a window procedure or a handle to a dialog box
        // procedure.
        //
        w_ = h;
        p_ = reinterpret_cast<WNDPROC> (
          SetWindowLongPtrW (h,
                             GWLP_WNDPROC,
                             reinterpret_cast<LONG_PTR> (subclass)));

        if (!p_)
        {
          std::cerr << "error: unable to inject subclass" << std::endl;
          free ();
          return;
        }

        d_ = d;
        skip_ = true;

        // We register these once to handle D3D device lost/reset events.
        // When a device resets (e.g. user changes resolution), all default
        // pool resources (vertex buffers, etc.) become invalid. We must
        // invalidate ImGui's cache and let it recreate them, otherwise
        // we'll crash on the next DrawPrimitive call.
        //
        if (!rst_)
        {
          r::on_reset (
            [] () { ImGui_ImplDX9_InvalidateDeviceObjects (); },
            [] () { ImGui_ImplDX9_CreateDeviceObjects (); }
          );
          rst_ = true;
        }
      }

      void
      draw (IDirect3DDevice9* d)
      {
        if (!d)
          throw std::invalid_argument ("null device");

        HWND h (find_wnd (d));

        // If the device pointer or the window handle has drifted since the
        // last frame (common during engine mode switches), the existing
        // setup is invalid. Re-initialize the entire stack.
        //
        if (d != d_ || (h != nullptr && h != w_))
          init (d);

        if (!d_)
          return;

        if (skip_)
        {
          skip_ = false;
          return;
        }

        state s (d);
        if (!s)
          return;

        ImGui_ImplDX9_NewFrame ();
        ImGui_ImplWin32_NewFrame ();
        ImGui::NewFrame ();

        ImGui::Render ();
        ImGui_ImplDX9_RenderDrawData (ImGui::GetDrawData ());
      }
    }

    void
    imgui ()
    {
      r::on_frame (draw);
    }
  }
}
