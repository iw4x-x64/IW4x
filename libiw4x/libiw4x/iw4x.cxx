#include <libiw4x/iw4x.hxx>
#include <libiw4x/win32/process-threads-api.hxx>

#include <array>
#include <string>
#include <cstring>

#include <MinHook.h>

using namespace std;

// MinGW normally calls _CRT_INIT() from DllMainCRTStartup. The
// unfortunate part of this arrangement for us is that this also means
// running the global constructors while the loader lock is held.
//
// And constructors are really the problematic bit here. There is
// nothing that prevents one from starting a thread and then waiting for
// it, while that thread may in turn need the loader lock before it can
// get anywhere. At which point neither side is going anywhere.
//
// So what we do instead is let DllMain do just enough to get us out of
// there: remember the module and patch the host entry point. The main
// thread will hit our lambda detour shortly afterwards and, by then,
// the loader has finished calling DllMain. This is where we call
// _CRT_INIT() and let MinGW carry on with its usual DLL startup.
//
// Note that we don't want to pick apart _CRT_INIT() and only run the
// bits that happen to be interesting to us (constructors being the
// obvious one). MinGW also records runtime state here which it expects
// to see again on DLL_PROCESS_DETACH. So calling the real thing keeps
// its startup and shutdown machinery matched without us having to know,
// and then maintain, the details of that contract.
//
extern "C" BOOL WINAPI _CRT_INIT (HANDLE, DWORD, LPVOID);

namespace iw4x
{
  constinit static HMODULE module_ (nullptr);

  HMODULE
  module () noexcept
  {
    return module_;
  }

  extern "C"
  {
    BOOL WINAPI
    DllMain (HINSTANCE h, DWORD r, LPVOID)
    {
      if (r != DLL_PROCESS_ATTACH)
        return TRUE;

      module_ = h;

      // There is a bit of a problem with doing our initialization from here.
      // DllMain is called with the loader lock held and the initialization we
      // are eventually going to do is far too involved to sensibly run under
      // it (and will almost certainly grow more involved over time).
      //
      // Luckily we are early enough in the process startup that we can defer
      // all this to the main thread. Specifically, the host is about to enter
      // its CRT startup and so we replace that entry with a small thunk of our
      // own. Once we get there DllMain has returned and we can finish setting
      // things up before continuing where the host would have gone normally.
      //
      // Note that these addresses are naturally tied to the exact host image
      // we support. Keeping them here also makes it fairly obvious what needs
      // revisiting if that image changes.
      //
      uintptr_t target (0x140358EBC);
      uintptr_t source (reinterpret_cast<uintptr_t> (+[] ()
      {
        // The instruction we replaced would normally get us into the CRT
        // startup sequence which, among other things, initializes the security
        // cookie before running any user code. Since we got in front of that
        // sequence, do this part ourselves first.
        //
        // __security_init_cookie
        //
        reinterpret_cast <void (*) ()> (0x1403598CC) ();

        // Now let MinGW finish setting up the DLL runtime. Normally
        // DllMainCRTStartup calls _CRT_INIT() before eventually calling
        // DllMain. We bypass that entry sequence, so this part is ours
        // to do explicitly.
        //
        // In particular, this is what makes the runtime state expected
        // by the rest of libiw4x available before we start using it. If
        // MinGW cannot complete that setup then continuing into IW4x
        // would leave us in a state the compiler-generated code was
        // never meant to run in.
        //
        // So treat failure here as an initialization failure and stop
        // before any of the normal IW4x startup gets a chance to run.
        //
        if (_CRT_INIT (module_, DLL_PROCESS_ATTACH, nullptr) == FALSE)
        {
          MessageBox (nullptr,
                      "unable to complete MinGW runtime startup",
                      "error",
                      MB_ICONERROR);
          exit (1);
        }

        // Next make ourselves permanent. Once IW4x starts installing hooks and
        // handing its addresses to the host, unloading the DLL is no longer a
        // meaningful operation. In fact, doing so would leave the process full
        // of references into an image that no longer exists.
        //
        // GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS saves us from having to know
        // our file name here: DllMain is an address in this module and Windows
        // can find the corresponding HMODULE from that. PIN then makes sure
        // its reference count can never reach zero.
        //
        // Failed that, there isn't really anything useful we can do. We haven't
        // started IW4x yet and continuing would violate an assumption made by
        // pretty much everything that follows, so report the problem and get
        // out.
        //
        HMODULE m;
        if (!GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_PIN |
                                  GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                                reinterpret_cast <LPCTSTR> (DllMain),
                                &m))
        {
          MessageBox (nullptr,
                      "unable to mark module as permanent",
                      "error",
                      MB_ICONERROR);
          exit (1);
        }

        // Now sort out the current directory. Normally Windows simply leaves
        // us with whatever directory the process inherited from whoever
        // started it. That may be fine when the executable is started directly
        // from its installation directory, but it is not something we can rely
        // on once launchers and other ways of starting the game enter the
        // picture.
        //
        // We want relative paths used by IW4x to start at the directory
        // containing libiw4x.dll, so find our module path and chop off the file
        // name.
        //
        // Note that passing NULL to GetModuleFileNameW() would give us the
        // executable path. That happens to produce the same directory in the
        // simple case and is exactly the kind of thing that can become rather
        // confusing once some other executable is involved in starting us.
        //
        // __ImageBase gives us the base of this image, which is also a valid
        // HMODULE for GetModuleFileNameW(). So use that and make the choice
        // explicit.
        //
        wchar_t p[MAX_PATH];
        if (GetModuleFileNameW (reinterpret_cast <HMODULE> (&__ImageBase),
                                p,
                                MAX_PATH))
        {
          // We only need the directory and the buffer is ours, so trim the
          // path in place. There should of course be a backslash in an absolute
          // module path, but check it since without one we don't have anything
          // sensible to pass to SetCurrentDirectoryW().
          //
          wchar_t* e (wcsrchr (p, L'\\'));

          if (e == nullptr || (*e = L'\0', !SetCurrentDirectoryW (p)))
          {
            MessageBox (nullptr,
                        "unable to set process current directory",
                        "error",
                        MB_ICONERROR);
            exit (1);
          }
        }
        else
        {
          MessageBox (nullptr,
                      "unable to retrieve module location",
                      "error",
                      MB_ICONERROR);
          exit (1);
        }

        // Set up the hooks before we hand control back to the host.
        // There will eventually be quite a few of these, so keep them
        // together here where it is easy to see which parts of the
        // process we take over during startup.
        //
        // MinHook lets us create everything first and then enable the
        // complete set at once. So this is also the place to add the
        // other hooks as they appear.
        //
        MH_Initialize ();

        MH_CreateHookApi (
          L"kernel32.dll", "ExitProcess",
          reinterpret_cast <LPVOID>  (&win32::exit_process),
          win32::exit_process_original ());

        MH_EnableHook (MH_ALL_HOOKS);

        // And with our early setup out of the way, continue with the CRT entry
        // point the host was going to use. From here startup proceeds normally
        // (except, of course, that IW4x now got a chance to run first).
        //
        // __scrt_common_main_seh
        //
        return reinterpret_cast <int (*) ()> (0x140358D48) ();
      }));

      // Finally install the thunk above. This looks a little crude because we
      // are spelling out an x86-64 absolute jump by hand, but there isn't much
      // machinery involved and keeping the encoding visible here makes it
      // easier to see exactly what we put into the host.
      //
      // FF 25 is an indirect jump through RIP. With a zero displacement the
      // pointer immediately follows the six-byte instruction, giving us:
      //
      //   ff 25 00 00 00 00
      //   <64-bit source address>
      //
      // So the whole patch is 14 bytes. Initializing the array with the two
      // opcode bytes also zero-initializes the displacement and the address
      // slot, after which we only have to copy `source` into the latter.
      //
      DWORD o (0), p (0);

      auto t (reinterpret_cast <void*> (target));
      auto s (array <unsigned char, 14> {0xFF, 0x25});

      memcpy (s.data () + 6, &source, sizeof (source));

      // The target lives on an executable page which is normally read-only, so
      // make the bytes writable for the duration of the patch and remember the
      // old protection for the restore below.
      //
      // Note that we are still in DllMain here. If any of these operations
      // fail then we cannot install the entry path described above, and trying
      // to unwind the process normally while holding the loader lock would
      // only make this failure path considerably more adventurous. So use
      // __fastfail() and let Windows terminate us immediately.
      //
      if (!VirtualProtect (t, s.size (), PAGE_EXECUTE_READWRITE, &o))
        __fastfail (FAST_FAIL_FATAL_APP_EXIT);

      memcpy (t, s.data (), s.size ());

      // Put the page back the way it was. The patch is complete at this point
      // and there is no reason to leave host code writable afterwards.
      //
      if (!VirtualProtect (t, s.size (), o, &p))
        __fastfail (FAST_FAIL_FATAL_APP_EXIT);

      // Windows also requires the instruction cache to be flushed after
      // modifying executable code. It is tempting to treat this as an
      // afterthought since the bytes are already there, but until this succeeds
      // we cannot rely on the processor seeing those bytes when it reaches the
      // patched address.
      //
      if (!FlushInstructionCache (GetCurrentProcess (), t, s.size ()))
        __fastfail (FAST_FAIL_FATAL_APP_EXIT);

      // That's all we want to do while attached under the loader lock. The
      // actual work starts when the main thread reaches the patched entry
      // above.
      //
      return TRUE;
    }
  }
}
