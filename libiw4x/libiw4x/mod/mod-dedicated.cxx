#include <libiw4x/mod/mod-dedicated.hxx>

#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>

#include <libiw4x/detour.hxx>
#include <libiw4x/import.hxx>
#include <libiw4x/logger.hxx>
#include <libiw4x/memory.hxx>
#include <libiw4x/scheduler.hxx>
#include <libiw4x/utility-win32.hxx>

using namespace std;

namespace iw4x
{
  namespace discovery
  {
    class discovery_service;
  }

  namespace mod
  {
    discovery::discovery_service*
    active_discovery_service ();

    namespace
    {
      // Layout of the 112-byte (0x70) structure passed to the internal zone
      // loading dispatcher at 0x140031FD0.
      //
      //   r11  = rsp (before sub rsp, 0x98)
      //   rcx  = r11 - 0x78 .. rsp + 0x20   (structure base)
      //
      //   [rcx+0x04] dword = 1
      //   [rcx+0x08] dword = 1
      //   [rcx+0x0c] dword = 0x800
      //   [rcx+0x10] dword = 0x7ff
      //   [rcx+0x14] dword = 0x7fe
      //   [rcx+0x18] dword = 0x00a
      //   [rcx+0x1c] dword = 0x02c
      //   [rcx+0x20] qword = zone[0].name  (code_post_gfx_mp)
      //   [rcx+0x28] qword = zone[1].name  (localized_code_post_gfx_mp)
      //   [rcx+0x30] qword = zone[2].name  (ui_mp)
      //   [rcx+0x38] qword = zone[3].name  (localized_ui_mp)
      //   [rcx+0x40] qword = zone[4].name  (common_mp)
      //   [rcx+0x48] qword = zone[5].name  (localized_common_mp)
      //   [rcx+0x50] qword = zone[6].name  (patch_mp)
      //   [rcx+0x58] byte  = 1
      //   [rcx+0x5a] word  = 3
      //   [rcx+0x5c] byte  = 1
      //   [rcx+0x5e] word  = 3
      //   [rcx+0x60] dword = 0xFFFFFFFF
      //   [rcx+0x64] dword = 0x384  (900)
      //   [rcx+0x68] dword = 0x1c2  (450)
      //
#pragma pack(push, 1)
      struct zone_load_request
      {
        uint8_t pending;       // [0x00] Set to 1 by the dispatcher.
        uint8_t _pad0 [3];
        uint32_t field_04;     // [0x04] = 1.
        uint32_t field_08;     // [0x08] = 1.
        uint32_t field_0c;     // [0x0c] = 0x800.
        uint32_t field_10;     // [0x10] = 0x7ff.
        uint32_t field_14;     // [0x14] = 0x7fe.
        uint32_t field_18;     // [0x18] = 0x00a.
        uint32_t field_1c;     // [0x1c] = 0x02c.
        const char* names [7]; // [0x20..0x57] Zone name pointers.
        uint8_t flag_58;       // [0x58] = 1.
        uint8_t _pad1;
        uint16_t field_5a;     // [0x5a] = 3.
        uint8_t flag_5c;       // [0x5c] = 1.
        uint8_t _pad2;
        uint16_t field_5e;     // [0x5e] = 3.
        uint32_t field_60;     // [0x60] = 0xFFFFFFFF.
        uint32_t field_64;     // [0x64] = 0x384.
        uint32_t field_68;     // [0x68] = 0x1c2.
        uint32_t _pad3;        // [0x6c] Padding to 0x70.
      };
#pragma pack(pop)

      static_assert (sizeof (zone_load_request) == 0x70,
                     "zone_load_request must be exactly 112 bytes");

      // Internal dispatcher.
      //
      using post_gfx_zone_dispatcher_t = void (*) (zone_load_request*);
      inline post_gfx_zone_dispatcher_t post_gfx_zone_dispatcher (
        reinterpret_cast<post_gfx_zone_dispatcher_t> (0x140031FD0));

      // Dvar pointer written by 0x1400F8660 after zone load. We replicate that
      // call so that callers depending on this side-effect still work.
      //
      using dvar_op_t = void (*) (void*, int);
      inline dvar_op_t dvar_post_zone_op (
        reinterpret_cast<dvar_op_t> (0x1402896E0));
      inline void** dvar_post_zone_ptr (
        reinterpret_cast<void**> (0x141C34668));

      // Build and submit a zone_load_request.
      //
      // zone_names must point to an array of exactly `count` C strings. The
      // remaining name slots are left as nullptr (the dispatcher stops at the
      // first null pointer or after all slots are filled).
      //
      void
      load_zones (const char* const* zone_names, int count)
      {
        alignas (16) zone_load_request req {};

        req.field_04 = 1;
        req.field_08 = 1;
        req.field_0c = 0x800;
        req.field_10 = 0x7ff;
        req.field_14 = 0x7fe;
        req.field_18 = 0x00a;
        req.field_1c = 0x02c;
        req.flag_58 = 1;
        req.field_5a = 3;
        req.flag_5c = 1;
        req.field_5e = 3;
        req.field_60 = 0xFFFFFFFF;
        req.field_64 = 0x384;
        req.field_68 = 0x1c2;

        for (int i = 0; i < count && i < 7; ++i)
          req.names [i] = zone_names [i];

        post_gfx_zone_dispatcher (&req);

        // Replicate the dvar side-effect that follows zone loading in the
        // original 0x1400F8660.
        //
        if (*dvar_post_zone_ptr)
          dvar_post_zone_op (*dvar_post_zone_ptr, 0);
      }

      // Dedicated-server replacement for 0x1400F8660.
      //
      // The original function loads all seven post-gfx zones through
      // 0x140031FD0, which also initialises renderer dvars (r_ignore, vid_xpos,
      // r_gamma, …) as a side-effect. That renderer dvar init is harmless on a
      // headless server (the dvars get registered but nothing ever reads them),
      // so we reuse the same dispatcher but supply a zone list we fully
      // control.
      //
      void
      dedicated_post_gfx_zone_loader ()
      {
        static const char* zones [] {
          "code_post_gfx_mp",
          "localized_code_post_gfx_mp",
          "ui_mp",
          "localized_ui_mp",
          "common_mp",
          "localized_common_mp",
          "patch_mp",
        };

        load_zones (zones, 7);

        R_LoadGraphicsAssets ();
      }

      using Cbuf_AddText_t = void (*) (int local_client_num, const char* text);
      inline Cbuf_AddText_t Cbuf_AddText (
        reinterpret_cast<Cbuf_AddText_t> (0x1401EC4A0));

      using Dvar_RegisterEnum_t = dvar* (*) (const char* name,
                                             const char** value_names,
                                             unsigned int flags,
                                             int default_index,
                                             const char* description);
      inline Dvar_RegisterEnum_t Dvar_RegisterEnum (
        reinterpret_cast<Dvar_RegisterEnum_t> (0x140287FC0));

      // Initialization-complete flag.
      //
      // Set to 1 by Com_Init (at 0x1401FB0E0) immediately after registering
      // the "motd" dvar, which is the last observable action of Com_Init.
      //
      auto& init_complete (*reinterpret_cast<int32_t*> (0x141C34D6C));

      // cached after registration so the heartbeat helper can read it without
      // doing a string lookup on every tick.
      //
      dvar* sv_lan_only_dvar (nullptr);

      // Enqueue a command with a trailing newline so Cbuf_AddText treats it as
      // a complete command token.
      //
      void
      exec (const char* cmd)
      {
        string s (cmd);
        if (s.empty () || s.back () != '\n')
          s += '\n';
        Cbuf_AddText (0, s.c_str ());
      }

      void
      post_init ()
      {
        Dvar_RegisterString (
          "sv_motd",
          "",
          0,
          "Message of the day displayed in the server browser");

        // sv_lanOnly: when true the server suppresses master-server heartbeats
        // and only shows up on the LAN server list.
        //
        sv_lan_only_dvar = Dvar_RegisterBool (
          "sv_lanOnly",
          false,
          0,
          "Suppress master-server heartbeats (LAN-only mode)");

        // Values mirror the x86 iw4x convention:
        //   0 = listen server
        //   1 = dedicated LAN server
        //   2 = dedicated internet server  (default here)
        //
        static const char* dedicated_values [] {
          "listen server",
          "dedicated LAN server",
          "dedicated internet server",
          nullptr,
        };

        Dvar_RegisterEnum ("dedicated",
                           dedicated_values,
                           /* flags       = DVAR_ROM */ 0x10,
                           /* default_idx = internet dedicated */ 2,
                           "True if this is a dedicated server");

        // An optional per-server autoexec that operators can use to set
        // sv_hostname, rcon_password, map rotations, etc. The engine does not
        // error if the file is absent.
        //
        exec ("exec autoexec.cfg");

        // Bring the networking layer into online / XBLive mode. Without these
        // the party and match-making systems refuse to start.
        //
        exec ("onlinegame 1");
        exec ("exec default_xboxlive.cfg");

        // Tell the session layer we are running a private, ranked online match
        // so that connecting clients get the right privileges.
        //
        exec ("xblive_rankedmatch 1");
        exec ("xblive_privatematch 1");
        exec ("xblive_privateserver 0");

        // Start the dedicated server session. xstartprivatematch is the
        // headless equivalent of xstartlobby used by regular clients and is
        // what the x86 iw4x dedicated server used as well.
        //
        exec ("xstartprivatematch");

        // Tune the network tick rates to sensible server defaults.
        //
        exec ("cl_maxpackets 125");
        exec ("snaps 30");
        exec ("com_maxfps 125");

        // Process any +exec / +map / +set commands the operator passed on the
        // command line (e.g. +exec server.cfg +map mp_rust).
        //
        // We do this last so that operator-supplied values override the
        // defaults set above.
        //
        // Com_AddStartupCommands ();
      }
    }

    // TODO: Replace with .cli
    //
    bool dedicated_module::
    is_dedicated ()
    {
      static bool result ([] ()
      {
        const char* cmd (GetCommandLineA ());
        if (cmd == nullptr)
          return false;

        // Accept "--dedicated" (iw4x long-form) and "-dedicated" (classic CoD
        // short-form). We check for a word boundary before "-dedicated" so that
        // a hypothetical dvar name like "non-dedicated" does not accidentally
        // match.
        //
        if (strstr (cmd, "--dedicated") != nullptr)
          return true;

        // Walk the string to find " -dedicated" or "\t-dedicated" so we do not
        // match a substring of another word.
        //
        const char* p (cmd);
        while ((p = strstr (p, "-dedicated")) != nullptr)
        {
          // The character immediately preceding the '-' must be whitespace.
          //
          if (p == cmd || p [-1] == ' ' || p [-1] == '\t')
            return true;
          ++p;
        }
        return false;
      }());
      return result;
    }

    // Party member auxiliary-info blob copied by the caller from:
    //
    //   [0x00..0x0f] xmmword
    //   [0x10..0x1f] xmmword
    //   [0x20..0x23] dword
    //   [0x24]       byte
    //
#pragma pack(push, 1)

    struct party_member_aux_info
    {
      std::uint8_t bytes [0x25];
    };

#pragma pack(pop)

    static_assert (sizeof (party_member_aux_info) == 0x25,
                   "party_member_aux_info must be 0x25 bytes");

    using party_member_aux_provider_t = party_member_aux_info* (*) ();

    inline party_member_aux_provider_t party_member_aux_provider (
      reinterpret_cast<party_member_aux_provider_t> (0x1402AA290));

    party_member_aux_info dedicated_party_member_aux_info {};

    party_member_aux_info*
    dedicated_party_member_aux_provider ()
    {
      return &dedicated_party_member_aux_info;
    }

    using netaddr_init_t = void (*) (std::uint32_t*, int);

    inline netaddr_init_t netaddr_init (
      reinterpret_cast<netaddr_init_t> (0x1401FC2F0));

    void
    dedicated_netaddr_init (std::uint32_t* p, int type)
    {
      // Preserve the observable contract of the original helper: the first
      // dword always stores the address type.
      //
      p [0] = static_cast<std::uint32_t> (type);

      // The original helper calls 0x1401317E0 for types 0 and 2 and stores its
      // result at +0x14. In dedicated mode that helper can reach an invalid
      // execute target, which suggests dependence on a client-side callback /
      // thunk chain that is not valid in headless execution.
      //
      // What we do here is deliberately collapse that derived field to a stable
      // zero/default value rather than entering the helper. The remaining
      // address fields (IP, port, etc.) are written by the caller immediately
      // afterwards and are the data that actually matter for the local loopback
      // path we are forcing.
      //
      *reinterpret_cast<std::uint32_t*> (reinterpret_cast<std::uint8_t*> (p) +
                                         0x14) = 0;
    }

    using live_get_local_nat_type_t = int (*) ();

    inline live_get_local_nat_type_t live_get_local_nat_type (
      reinterpret_cast<live_get_local_nat_type_t> (0x1402A67E0));

    int
    dedicated_live_get_local_nat_type ()
    {
      return 3;
    }

    using live_get_xuid_t = std::uint64_t (*) (int);

    inline live_get_xuid_t live_get_xuid (
      reinterpret_cast<live_get_xuid_t> (0x1402A6930));

    std::uint64_t
    dedicated_live_get_xuid (int)
    {
      return 0;
    }

    using subsystem_pump_t = void (*) ();

    inline subsystem_pump_t subsystem_pump (
      reinterpret_cast<subsystem_pump_t> (0x140030AC0));

    inline void*& subsystem_object (
      *reinterpret_cast<void**> (0x148D45E48));

    void
    dedicated_subsystem_pump ()
    {
      // In normal client mode this routine drives a global interface object via
      // vtable calls at 0x30 and 0x38. Dedicated mode can reach this path with
      // the global object absent, in which case the original code dereferences
      // a null pointer immediately. Instead, treat object absence as "nothing
      // to pump".
      //
      if (subsystem_object == nullptr)
        return;

      subsystem_pump ();
    }

    dedicated_module::
    dedicated_module ()
    {
      if (!is_dedicated ())
        return;

      // Replace post-gfx zone loader (0x1400F8660) with our own.
      //
      static auto zone_loader_target (
        reinterpret_cast<void (*) ()> (0x1400F8660));
      detour (zone_loader_target, dedicated_post_gfx_zone_loader);

      // NOP the call to the "CoD Splash Screen" window-class creator
      // (0x1402AA510). On a headless server the function would attempt to load
      // splash.bmp and register a window class we will never use.
      //
      memory::write (0x1402A9375, 0x90, 5);

      // NOP the call to ShowWindow / UpdateWindow on the splash HWND
      // (0x1402AA7E0). Without the window the show call would dereference a
      // null handle.
      //
      memory::write (0x1402A937A, 0x90, 5);

      // We also need to NOP out a bunch of screen update and rendering paths
      // that the engine inadvertently reaches even in headless mode.
      //
      // Without an initialised UI and rendering subsystem, letting the engine
      // wander down these paths usually ends in an immediate null pointer
      // dereference. So, we stub them out to keep the server loop from
      // imploding.
      //
      memory::write (0x1401F9E4A, 0x90, 5);
      memory::write (0x1400B437C, 0x90, 5);
      memory::write (0x1400FE80C, 0x90, 5);
      memory::write (0x1400FE3B6, 0x90, 5);
      memory::write (0x1400FE3BB, 0x90, 5);
      memory::write (0x1400FE3EF, 0x90, 5);
      memory::write (0x1400FE451, 0x90, 5);
      memory::write (0x1400FE478, 0x90, 5);
      memory::write (0x1400FE49F, 0x90, 5);
      memory::write (0x1400FE4CB, 0x90, 5);
      memory::write (0x1400FE50B, 0x90, 5);
      memory::write (0x1400FE681, 0x90, 5);
      memory::write (0x1400F0A73, 0x90, 5);
      memory::write (0x1400F0C7D, 0x90, 5);
      memory::write (0x1400F6F0E, 0x90, 5);
      memory::write (0x1400F6F52, 0x90, 5);
      memory::write (0x14001B259, 0x90, 5);
      memory::write (0x14001B2C5, 0x90, 5);
      memory::write (0x14001B337, 0x90, 5);
      memory::write (0x14001B508, 0x90, 5);
      memory::write (0x14041F744, 0x90, 5);
      memory::write (0x14041F754, 0x90, 5);
      memory::write (0x1494F6EB8, 0x90, 5);
      memory::write (0x1494F6EC4, 0x90, 5);
      memory::write (0x1400FE836, 0x90, 5);

      // NOP the call to the sound-system initialiser (0x1402C33B0).
      //
      // That function calls CoInitializeEx, initialises X3DAudio, registers
      // audio dvars, and starts the Miles/XAudio2 backend, none of which make
      // sense on a headless server.
      //
      memory::write (0x1401FB064, 0x90, 5);

      // NOP the call to Snd_InitDvars (0x14024AD10).
      //
      // Despite its name this function does two things: it registers the
      // sound-related dvars (snd_volume, snd_enable2D/3D, etc.) AND it attempts
      // to load "soundaliases/channels.def" out of the zone system. When no
      // sound zones are present on a dedicated server the asset lookup returns
      // null and the engine immediately calls Com_Error:
      //
      //   "unable to load entity channel file [soundaliases/channels.def]"
      //
      // Neither the dvars nor the channel file are needed on a headless server,
      // so we suppress the whole call.
      //
      memory::write (0x1401FB073, 0x90, 5);

      // NOP the call that spawns the render thread (0x140031450).
      //
      memory::write (0x1401FB069, 0x90, 5);

      // NOP the call to the renderer material machinery (0x1400F8740) in the
      // Com_Init call sequence.
      //
      memory::write (0x1401FB06E, 0x90, 5);

      // Stub out R_Init (0x1400F8740) itself with a single RET.
      //
      // R_Init is reachable through at least three independent paths in
      // dedicated mode:
      //
      //   1. Direct call from Com_Init   (0x1401FB06E) - already NOP'd above.
      //   2. Guarded reinit wrapper      (0x1401FC140) - calls R_Init when a
      //      "renderer_initialized" flag at 0x140CA7CB8 is 0.
      //   3. Deferred vid-restart path   (0x14023FA30 .. 0x1401FC050) - set by
      //      a pending-restart flag and executed each Com_Frame tick.
      //
      // Placing a RET at the function prologue means every caller, including
      // any we have not yet identified, returns harmlessly without touching
      // Direct3D or allocating GPU resources.
      //
      // The sub rsp has not yet executed when we return, so the stack pointer
      // is still balanced and the return address at [rsp] is valid.
      //
      memory::write (0x1400F8740, 0xC3, 1);

      // Patch the default value of r_loadForRenderer from true (0x01) to false
      // (0x00).
      //
      // When r_loadForRenderer is true, the zone-loading callbacks for renderer
      // asset types (techsets, materials, shaders…) attempt to allocate GPU
      // resources as each asset is registered. With no D3D device present those
      // allocations fail, and the asset is either never inserted into the hash
      // table or inserted in a broken state, making DB_FindXAssetHeader return
      // null for assets that were physically present in the loaded zone.
      //
      // When r_loadForRenderer is false the engine's own description applies:
      // "Set to false to disable dx allocations (for dedicated server mode)".
      // Every asset is then registered as a stub entry without touching
      // Direct3D, so DB_FindXAssetHeader finds it correctly.
      //
      memory::write (0x14002D66F, 0x00, 1);

      // Pre-set the "renderer already initialized" flag at 0x140CA7CB8.
      //
      // R_Init normally writes 1 here as its very first action (0x1400F874B:
      // mov dword [0x140CA7CB8], 1). Because we stub R_Init with RET it never
      // runs, so the flag stays 0. The guarded reinit wrapper at 0x1401FC140
      // checks this flag and calls R_Init whenever it is 0, which would be on
      // every Com_Frame tick, defeating our stub.
      //
      // Writing 1 here makes the guard believe R_Init already completed,
      // permanently suppressing path 2 above without any per-frame overhead.
      //
      memory::write (0x140CA7CB8, 0x01, 1);

      // The member-info update path at 0x14010B372 calls 0x1402AA290 and then
      // immediately copies 0x25 bytes from the returned pointer into the party
      // member slot without checking for null. On a normal client this appears
      // to be a valid assumption because Live / presence state exists. In
      // dedicated mode under Wine, however, that provider may legitimately
      // yield null and the subsequent movups [rax] faults.
      //
      // Rather than NOPing the copy sequence, preserve the expected contract
      // and redirect null returns to a stable zero/default object.
      //
      detour (party_member_aux_provider, dedicated_party_member_aux_provider);

      // 0x1401FC2F0 initialises a small address structure and, for address
      // types 0 and 2, consults 0x1401317E0 to derive an auxiliary field at
      // +0x14. Under Wine in dedicated mode that helper can enter an invalid
      // execute target and fault. The dedicated server does not need that
      // client-side derivation for its forced loopback path, so replace the
      // helper with a version that writes a stable default instead.
      //
      detour (netaddr_init, dedicated_netaddr_init);
      detour (live_get_local_nat_type, dedicated_live_get_local_nat_type);
      detour (live_get_xuid, dedicated_live_get_xuid);
      detour (subsystem_pump, dedicated_subsystem_pump);

      // Schedule the startup sequence to fire once Com_Init has finished. The
      // predicate polls init_complete (0x141C34D6C) which Com_Init sets to 1 as
      // its very last action.
      //
      scheduler::post (com_frame_domain,
                       post_init,
                       repeat_until_predicate {[]
      {
        return init_complete != 0;
      }});
    }
  }
}
