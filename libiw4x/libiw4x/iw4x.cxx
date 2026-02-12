#include <libiw4x/iw4x.hxx>

#include <libiw4x/context.hxx>
#include <libiw4x/memory.hxx>
#include <libiw4x/scheduler.hxx>

#include <libiw4x/client/init.hxx>
#include <libiw4x/ui/init.hxx>
#include <libiw4x/win32/init.hxx>
#include <libiw4x/windows/init.hxx>

namespace iw4x
{
  // We avoid a pointer-based singleton here to prevent the compiler from
  // generating dependent loads. That is, even if the pointer is TU-local, the
  // compiler assumes it might change, preventing it from folding the access
  // into a single address computation.
  //
  // Instead, we reserve storage with static duration and use placement-new.
  // This gives us a reference with a fixed base address, which is critical
  // for performance in our per-frame detours (avoids the pointer-chasing
  // dependency chan).
  //
  // Note that we must manually sequence construction since the linker only
  // sees the storage, not the object lifetime.
  //
  alignas (context) std::byte ctx_storage [sizeof (context)];
  context& ctx (reinterpret_cast<context&> (ctx_storage));

  namespace
  {
    void
    attach_console ()
    {
      // The subtlety here is that Windows has many ways to end up with stdout
      // and stderr pointing *somewhere* (sometimes to an actual console,
      // sometimes to a pipe, sometimes to a completely invalid handle). We do
      // not want to attach to `CONOUT$` in cases where the existing handles are
      // already valid and intentional, as this would silently discard the real
      // output sink.
      //
      // Instead, we first check `_fileno(stdout)` and `_fileno(stderr)`. That
      // is, MSVCRT sets these up at startup and will return `-2`
      // (`_NO_CONSOLE_FILENO`) if the file is invalid. This check is more
      // trustworthy than calling `GetStdHandle()`, which can return stale
      // handle IDs that may already be reused for unrelated objects by the time
      // we run.
      //
      if (_fileno (stdout) >= 0 || _fileno (stderr) >= 0)
      {
        // If either `_fileno()` is valid, we go one step further: `_fileno()`
        // itself had a bug (http://crbug.com/358267) in SUBSYSTEM:WINDOWS
        // builds for certain MSVC versions (VS2010 to VS2013), so we
        // double-check by calling `_get_osfhandle()` to confirm that the
        // underlying OS handle is valid. Only if both streams are invalid do
        // we attempt to attach a console.
        //
        intptr_t stdout_handle (_get_osfhandle (_fileno (stdout)));
        intptr_t stderr_handle (_get_osfhandle (_fileno (stderr)));

        if (stdout_handle >= 0 || stderr_handle >= 0)
          return;
      }

      // At this point, both standard streams appear invalid, so we attempt to
      // attach to the parent's console. Note that this call may fail for
      // expected reasons such as being already attached or the parent having
      // exited, and in all such cases the failure is non-fatal and we simply
      // bail out.
      //
      if (AttachConsole (ATTACH_PARENT_PROCESS) != 0)
      {
        // Once attached, rebind `stdout` and `stderr` to `CONOUT$` using
        // `freopen()`. Also duplicate their low-level descriptors (1 for
        // stdout, 2 for stderr) so that code using the raw file descriptor API
        // observes the same handles.
        //
        // Note that failure to rebind is non-fatal. Console output is
        // diagnostic-only and has no bearing on core functionality. We
        // therefore avoid exceptions and suppress all errors unconditionally.
        //
        bool stdout_rebound (false);
        bool stderr_rebound (false);

        if (freopen ("CONOUT$", "w", stdout) != nullptr &&
            _dup2 (_fileno (stdout), 1) != -1)
          stdout_rebound = true;

        if (freopen ("CONOUT$", "w", stderr) != nullptr &&
            _dup2 (_fileno (stderr), 2) != -1)
          stderr_rebound = true;

        // If stream were rebound, realign iostream objects (`cout`, `cerr`,
        // etc.) with C FILE streams.
        //
        if (stdout_rebound && stderr_rebound)
          ios::sync_with_stdio ();
      }
    }
  }

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

        attach_console ();

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
        // Note that we don't pass NULL to GetModuleFileName() as it incorrectly
        // returns the path of the process executable. That is, if we are
        // running under a generic launcher or wrapper, that would be the
        // launcher's path, not ours, and any relative path resolution based on
        // it would be incorrect (i.e., we would look for configuration files
        // next to the launcher).
        //
        // Instead, we use the __ImageBase MSVC linker pseudo-variable. Its
        // address coincides with the module's base address (HMODULE) which in
        // turns can be used to query IW4x path regardless of who the hosting
        // process is.
        //
        char p [MAX_PATH];
        if (GetModuleFileName (reinterpret_cast<HMODULE> (&__ImageBase),
                               p,
                               MAX_PATH))
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

        // Windows has this notion of EcoQoS (Power Throttling) which
        // effectively throttles the CPU for background processes. Since we
        // often look like one to the OS, we need to explicitly opt-out to get
        // full performance.
        //
        PROCESS_POWER_THROTTLING_STATE pt {};

        pt.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
        pt.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
        pt.StateMask = 0;

        // Try to apply the settings. If the OS refuses this for the current
        // process (unlikely, but maybe rights issues or a very weird
        // environment), bail out as we can't guarantee the performance
        // characteristics we need.
        //
        if (!SetProcessInformation (GetCurrentProcess (),
                                    ProcessPowerThrottling,
                                    &pt,
                                    sizeof (pt)))
        {
          MessageBox (nullptr,
                      "unable to disable power throttling",
                      "error",
                      MB_ICONERROR);
          exit (1);
        }

        // Start Quill backend thread.
        //
        // Note that Quill backend is kept responsive (no sleep) and is allowed
        // to exit without draining queues to avoid MinGW-specific shutdown
        // hangs.
        //
        quill::Backend::start ({
          .enable_yield_when_idle               = true,
          .sleep_duration                       = 0ns,
          .wait_for_queues_to_empty_before_exit = false,
          .check_printable_char                 = {},
          .log_level_short_codes                =
          {
            "3", "2", "1", "D", "I", "N", "W", "E", "C", "B", "_"
          }
        });

        // Configure Quill log layout.
        //
        // @@: It may be tempting to include the name of the calling function
        // in log output, but this is not reliable in practice. In particular,
        // calls made from lambdas (and other compiler-generated contexts)
        // often yield generic names.
        //
        quill::PatternFormatterOptions format_options {
          "%(time) [%(log_level_short_code)] %(message)",
          "%H:%M:%S.%Qms",
          quill::Timezone::LocalTime,
        };

        // Create the main logger instance.
        //
        // Note that its lifetime is managed by Quill.
        //
        quill::Logger* l (
          quill::Frontend::create_or_get_logger (
            "iw4x",
            quill::Frontend::create_or_get_sink<quill::ConsoleSink> ("cs"),
            format_options));

        // In development builds, enable the most verbose tracing level.
        //
#if LIBIW4X_DEVELOP
        l->set_log_level (quill::LogLevel::TraceL3);
#endif

        scheduler s;
        new (&ctx_storage) context (s, l);

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
        memwrite (0x1400F85F8, 0x90, 26);                                       // Suppress Cmd_AddCommandInternal for CL_Live_BaseGameLicenseCheck

        // Patch s_cpuCount to use hardware concurrency in Sys_InitMainThread.
        //
        // Note that this may violate implicit engine assumptions about CPU
        // topology. Current behavior is stable, but should be monitored for
        // regressions.
        //
        *(uint32_t*) 0x14020DD06 = thread::hardware_concurrency ();

        // Experimental offline mode.
        //
        // memwrite (0x1402A5F70, 0x90, 3);                                        // xboxlive_signed
        // memwrite (0x1402A5F73, 0x74, 1);                                        // ^
        // memwrite (0x1400F5B86, 0xEB, 1);                                        // ^
        // memwrite (0x1400F5BAC, 0xEB, 1);                                        // ^
        // memwrite (0x14010B332, 0xEB, 1);                                        // ^
        // memwrite (0x1401BA1FE, 0xEB, 1);                                        // ^
        // memwrite (0x140271ED0, 0xC3, 1);                                        // playlist
        // memwrite (0x1400F6BC4, 0x90, 2);                                        // ^
        // memwrite (0x1400FC833, 0xEB, 1);                                        // configstring
        // memwrite (0x1400D2AFC, 0x90, 2);                                        // ^
        // memwrite (0x1400E4DA0, 0x33, 1);                                        // stats
        // memwrite (0x1400E4DA1, 0xC0, 1);                                        // ^
        // memwrite (0x1400E4DA2, 0xC3, 1);                                        // ^

        client::init ();
        ui::init ();
        win32::init ();
        windows::init ();

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
      // already pre-patched with ASLR disabled, so VirtualProtect should
      // always succeed. If it does fail, we bail out rather than attempting
      // to write to a potentially protected page.
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

    // Force 'High Performance Graphics'.
    //
    // Official documentation states that this mechanism is not supported when
    // invoked from a DLL. Turn out that in practice, user reports and field
    // testing indicate that it does actually take effect and is in fact
    // required for hybrid (Optimus) system. We therefore enable it here despite
    // the documented limitation.
    //
    // https://docs.nvidia.com/gameworks/content/technologies/desktop/optimus
    // https://gpuopen.com/learn/amdpowerxpressrequesthighperformance/
    //
    // @@: AmdPowerXpressRequestHighPerformance has a history of causing
    // instability in some games. While we currently enable it,
    // this should be treated as a potential fault surface. If IW4x
    // exhibits similar crash patterns, the correct response is likely
    // to introduce a conditional or opt-out rather than remove it
    // outright.
    //
    LIBIW4X_SYMEXPORT DWORD NvOptimusEnablement = 0x00000001;
    LIBIW4X_SYMEXPORT DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
  }
}
