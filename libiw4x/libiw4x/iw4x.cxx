#include <libiw4x/iw4x.hxx>

#include <libiw4x/r/init.hxx>

#include <libiw4x/memory.hxx>

namespace iw4x
{
  extern "C"
  {
    BOOL WINAPI
    DllMain (HINSTANCE, DWORD r, LPVOID)
    {
      // We are only interested in the process attach event. That is, thread
      // notifications are just noise for our use case, and we handle cleanup
      // via static destructors rather than manual intervention on process
      // detach.
      //
      if (r != DLL_PROCESS_ATTACH)
        return TRUE;

      // DllMain executes while the process loader lock is held, so we defer
      // initialization to IW4's main thread to avoid ordering violation.
      //
      uintptr_t t (0x140358EBC);
      uintptr_t s (reinterpret_cast<uintptr_t> (+[] ()
      {
        // __security_init_cookie
        //
        reinterpret_cast<void (*) ()> (0x1403598CC) ();

        // Under normal circumstances, a DLL is unloaded via FreeLibrary once
        // its reference count reaches zero. This is acceptable for auxiliary
        // libraries but unsuitable for modules like ours, which embed deeply
        // into the host process.
        //
        HMODULE m;
        if (!GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_PIN |
                                  GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                                reinterpret_cast<LPCTSTR> (DllMain),
                                &m))
        {
          MessageBox (nullptr,
                      "unable to mark module as permanent",
                      "error",
                      MB_ICONERROR);
          exit (1);
        }

        // By default, the process inherits its working directory from whatever
        // environment or launcher invoked it, which may vary across setups and
        // lead to unpredictable relative path resolution.
        //
        // The strategy here is to explicitly realign the working directory to
        // the executable's own location. That is, we effectively makes all
        // relative file operations resolve against the executable's directory
        // when the process is hosted or started indirectly.
        //
        char p [MAX_PATH];
        if (GetModuleFileName (nullptr, p, MAX_PATH))
        {
          string s (p);
          size_t i (s.rfind ('\\'));

          if (i == string::npos ||
              !SetCurrentDirectory (s.substr (0, i).c_str ()))
          {
            MessageBox (nullptr,
                        "unable to set module current directory",
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

        // Relax module's memory protection to permit writes to segments that
        // are otherwise read-only.
        //
        MODULEINFO mi;
        if (GetModuleInformation (GetCurrentProcess (),
                                  GetModuleHandle (nullptr),
                                  &mi,
                                  sizeof (mi)))
        {
          DWORD o (0);
          if (!VirtualProtect (mi.lpBaseOfDll,
                               mi.SizeOfImage,
                               PAGE_EXECUTE_READWRITE,
                               &o))
          {
            MessageBox (nullptr,
                        "unable to change module protection",
                        "error",
                        MB_ICONERROR);
            exit (1);
          }
        }
        else
        {
          MessageBox (nullptr,
                      "unable to retrieve module information",
                      "error",
                      MB_ICONERROR);
          exit (1);
        }

        memwrite (0x1402A91E5, "\xB0\x01");                                     // Suppress XGameRuntimeInitialize call in WinMain
        memwrite (0x1402A91E7, 0x90, 3);                                        // ^
        memwrite (0x1402A6A4B, 0x90, 5);                                        // Suppress XCurl call in Live_Init
        memwrite (0x1402A6368, 0x90, 5);                                        // Suppress XCurl call in Live_Frame
        memwrite (0x1402A8CFE, 0x90, 5);                                        // Suppress GDK shutdown in Com_Quit_f
        memwrite (0x1402A92B3, 0x90, 13);                                       // Suppress Sys_CheckCrashOrRerun in WinMain
        memwrite (0x1401FAC87, 0x90, 7);                                        // Suppress com_safemode in Com_Init (only set by Sys_CheckCrashOrRerun).
        memwrite (0x1401FAC8E, 0xEB, 1);                                        // ^
        memwrite (0x1403B1E88, "iw4x.cfg");                                     // Rename "config_mp.cfg" to "iw4x.cfg"
        memwrite (0x1403B1E90, 0x00, 6);                                        // ^
        memwrite (0x1403B30B8, "// generated by IW4x, do not modify\n");        // Rename "Infinity Ward" to "IW4x"
        memwrite (0x1403B30DC, 0x00, 9);                                        // ^
        memwrite (0x1402864F0, "\xB0\x01\xC3");                                 // Patch Content_DoWeHaveContentPack to return true

        // Patch s_cpuCount to use hardware concurrency in Sys_InitMainThread.
        //
        // Note that this may violate implicit engine assumptions about CPU
        // topology. Current behavior is stable, but should be monitored for
        // regressions.
        //
        *(uint32_t*) 0x14020DD06 = thread::hardware_concurrency ();

        // Experimental offline mode.
        //
        memwrite (0x1402A5F70, 0x90, 3); // xboxlive_signed
        memwrite (0x1402A5F73, 0x74, 1); // ^
        memwrite (0x1400F5B86, 0xEB, 1); // ^
        memwrite (0x1400F5BAC, 0xEB, 1); // ^
        memwrite (0x14010B332, 0xEB, 1); // ^
        memwrite (0x1401BA1FE, 0xEB, 1); // ^
        memwrite (0x140271ED0, 0xC3, 1); // playlist
        memwrite (0x1400F6BC4, 0x90, 2); // ^
        memwrite (0x1400FC833, 0xEB, 1); // configstring
        memwrite (0x1400D2AFC, 0x90, 2); // ^
        memwrite (0x1400E4DA0, 0x33, 1); // stats
        memwrite (0x1400E4DA1, 0xC0, 1); // ^
        memwrite (0x1400E4DA2, 0xC3, 1); // ^

        // __scrt_common_main_seh
        //
        return reinterpret_cast<int (*) ()> (0x140358D48) ();
      }));

      // Construct a 64-bit absolute jump. x64 has no single instruction for
      // this, so we use `FF 25` (JMP [RIP+0]) followed by the 64-bit address.
      //
      array<unsigned char, 14> seq
      ({
        static_cast<unsigned char> (0xFF),
        static_cast<unsigned char> (0x25),
        static_cast<unsigned char> (0x00),
        static_cast<unsigned char> (0x00),
        static_cast<unsigned char> (0x00),
        static_cast<unsigned char> (0x00),
        static_cast<unsigned char> (s       & 0xFF),
        static_cast<unsigned char> (s >> 8  & 0xFF),
        static_cast<unsigned char> (s >> 16 & 0xFF),
        static_cast<unsigned char> (s >> 24 & 0xFF),
        static_cast<unsigned char> (s >> 32 & 0xFF),
        static_cast<unsigned char> (s >> 40 & 0xFF),
        static_cast<unsigned char> (s >> 48 & 0xFF),
        static_cast<unsigned char> (s >> 56 & 0xFF)
      });

      // A failure here would indicate that ASLR is still enabled for this
      // region. In practice this is not expected: the distributed binary is
      // already pre-patched with ASLR disabled, so VirtualProtect should always
      // succeed. If it does fail, we bail out rather than attempting to write
      // to a potentially protected page.
      //
      DWORD o (0);
      if (VirtualProtect (reinterpret_cast<void*> (t),
                          seq.size (),
                          PAGE_EXECUTE_READWRITE,
                          &o) != 0)
      {
        memmove (reinterpret_cast<void*> (t), seq.data (), seq.size ());
      }
      else
        return FALSE;

      // If we made it here, we are attached to the process.
      //
      return TRUE;
    }
  }
}
