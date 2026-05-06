#include <libiw4x/mod/mod-network.hxx>

#include <libiw4x/detour.hxx>
#include <libiw4x/logger.hxx>
#include <libiw4x/scheduler.hxx>

#include <libiw4x/mod/mod-oob.hxx>

using namespace std;

namespace iw4x
{
  namespace mod
  {
    // The engine maintains per-client transport objects starting at this base
    // address.
    //
    static constexpr uintptr_t dw_b (0x141a4a998);
    static constexpr size_t dw_s (0x30f8);
    static constexpr int mc (18);

    SOCKET bd_s (INVALID_SOCKET);
    auto* const ip_socket (reinterpret_cast<uint64_t*> (0x1467E8490));

    // Steal the Demonware (bdNet) socket for our own out-of-band traffic.
    //
    // During startup, bdNet binds a UDP socket on port 3074. Since this socket
    // already has a UPnP mapping negotiated on the user's router, remote peers
    // can reach us effortlessly on it. Rather than opening a secondary socket
    // (which would demand its own UPnP mapping), we just fish bdNet's socket
    // handle out of memory and plant it into the engine's global ip_socket.
    //
    void
    adopt_dw_s ()
    {
      void** pp (reinterpret_cast<void**> (0x1407149a8));

      if (*pp == nullptr)
        return;

      char* bd (static_cast<char*> (*pp));
      char* bds (*reinterpret_cast<char**> (bd + 0x10));

      if (bds == nullptr)
      {
        // The bdSocket is destroyed, likely because bdNet is restarting. We
        // invalidate our cached handle here so we can re-adopt once the new
        // socket appears.
        //
        if (bd_s != INVALID_SOCKET)
        {
          log::info << "demonware socket destroyed, invalidating cached handle";
          bd_s = INVALID_SOCKET;
          *ip_socket = 0;
        }

        return;
      }

      int32_t f (*reinterpret_cast<int32_t*> (bds + 0x08));

      if (f == -1)
        return;

      SOCKET ns (static_cast<SOCKET> (static_cast<uint32_t> (f)));

      // Nothing to do if the socket hasn't actually changed.
      //
      if (ns == bd_s)
        return;

      log::info << "adopting new demonware socket (fd: " << ns << ")";

      *ip_socket = static_cast<uint64_t> (static_cast<uint32_t> (f));
      bd_s = ns;
    }

    // Set up dummy transport blocks.
    //
    // In the stock game, the DW connect handshake populates transport slots
    // with pointers to bdConnection objects. Since we bypass DW, these slots
    // end up holding garbage pointers.
    //
    // If we just NULL them out, SV_DirectConnect won't crash, but
    // Netchan_Transmit will skip the full send path. So instead, we allocate
    // dummy zeroed blocks per client to trick the engine into safely
    // reading/writing the lock and sequence fields (at offsets 0x110 and 0x00)
    // without blowing up.
    //
    static constexpr size_t dsz (0x1000);
    static char dpool [mc * dsz] {};

    void
    setup_dums ()
    {
      log::debug << "setting up dummy transport blocks for " << mc
                 << " clients";

      for (int i (0); i < mc; ++i)
      {
        void** s (reinterpret_cast<void**> (dw_b + i * dw_s));

        // Just point the slot at its dedicated zeroed region.
        //
        *s = &dpool [i * dsz];
      }
    }

    // Extract the first whitespace-delimited token after the 4-byte OOB header
    // (0xFF 0xFF 0xFF 0xFF).
    //
    // @wroyca: We'll want to use oob:: instead.
    //
    string
    grep_oob (const char* d, int s)
    {
      if (s <= 4)
        return {};

      const char* b (d + 4);
      const char* e (d + s);

      // Skip any leading whitespace.
      //
      while (b < e && (*b == ' ' || *b == '\t' || *b == '\r'))
        ++b;

      const char* t (b);

      // Find the end of the token.
      //
      while (t < e && *t != ' ' && *t != '\t' && *t != '\r' && *t != '\n' &&
             *t != '\0')
        ++t;

      return string (b, t);
    }

    void
    sv_connectionless_packet (netadr_t* a, msg_t* m)
    {
      // We only really care about remote OOB packets, which are indicated by a
      // 0xFFFFFFFF header. That is, skip loopback traffic (type 2) so we don't
      // end up flooding the console with the host's internal chatter.
      //
      if (a->type != NA_LOOPBACK && m != nullptr &&
          m->data != nullptr && m->cursize > 4)
      {
        const unsigned char* h (
          reinterpret_cast<const unsigned char*> (m->data));

        bool o (h [0] == 0xFF && h [1] == 0xFF && h [2] == 0xFF &&
                h [3] == 0xFF);

        if (o)
        {
          string c (grep_oob (m->data, m->cursize));

          // Before the engine gets its hands on a 'connect' command, we need to
          // make sure we clean up those stale DW transport pointers.
          //
          if (c == "connect")
          {
            log::debug << "intercepted 'connect' oob command, cleaning up "
                          "transport pointers";
            setup_dums ();
          }

          // Dispatch to our global OOB pipeline.
          //
          // If the disposition dictates that the packet was consumed, we return
          // early to prevent the engine from logging a spurious unknown
          // command.
          //
          if (oob_dispatch (a, m))
          {
            log::trace_l1 << "dispatched and consumed oob command '" << c
                          << "'";
            return;
          }
        }
      }

      SV_ConnectionlessPacket (a, m);
    }

    // Bypass the engine's DW bdConnection path for IP sends.
    //
    // Normally, NET_OutOfBandPrint routes IP packets through bdConnectionStore,
    // which ends up adding DW framing. We intercept this here and use a plain
    // sendto() on the raw socket so the actual OOB traffic reaches the peer
    // undisturbed.
    //

    bool
    sys_send_packet (int l, const char* d, const netadr_t* a)
    {
      // The engine tags addresses from NET_GetPacket as BROADCAST (3) and keeps
      // them that way during the challenge/response phase. We need to intercept
      // both IP and BROADCAST so they go through our sendto path rather than
      // the engine's internal conversion.
      //
      if (a->type != NA_IP && a->type != NA_BROADCAST)
      {
        return Sys_SendPacket (l, d, a);
      }

      const uint8_t* ip (reinterpret_cast<const uint8_t*> (a->ip));

      // Don't bother intercepting unless we actually have an IP to send to.
      //
      if (ip [0] == 0 && ip [1] == 0 && ip [2] == 0 && ip [3] == 0)
      {
        return Sys_SendPacket (l, d, a);
      }

      SOCKET s (bd_s);

      if (s == INVALID_SOCKET || s == 0)
      {
        log::warning
          << "dropping outgoing oob packet: invalid demonware socket";
        return false;
      }

      sockaddr_in sa {};
      sa.sin_family = AF_INET;

      memcpy (&sa.sin_addr, a->ip, 4);

      sa.sin_port = a->port;

      log::trace_l3 << "sending raw oob packet (" << l
                    << " bytes) bypassing dw framing";

      int r (sendto (s,
                     static_cast<const char*> (d),
                     l,
                     0,
                     reinterpret_cast<sockaddr*> (&sa),
                     sizeof (sa)));

      if (r == SOCKET_ERROR)
      {
        log::error << "sendto failed for custom oob packet";
        return false;
      }

      return true;
    }

    network_module::network_module ()
    {
      log::info << "initializing network module";

      detour (SV_ConnectionlessPacket,  sv_connectionless_packet);
      detour (Sys_SendPacket,           sys_send_packet);

      scheduler::post (com_frame_domain,
                       []
      {
        adopt_dw_s ();
      }, repeat_every_tick);
    }
  }
}
