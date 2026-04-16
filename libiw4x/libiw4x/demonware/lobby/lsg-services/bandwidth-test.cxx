#include <libiw4x/demonware/lobby/lsg-services/bandwidth-test.hxx>

#include <cstdint>
#include <cstring>
#include <stdexcept>

#include <libiw4x/detour.hxx>
#include <libiw4x/import.hxx>
#include <libiw4x/logger.hxx>
#include <libiw4x/utility-win32.hxx>

using namespace std;

namespace iw4x
{
  namespace demonware
  {
    namespace
    {
      // bdBandwidthTestClient object layout.
      //
      //  0x00: vtable*
      //  0x10: controller_index (dword)
      //  0x14: initialized (dword, 0=no, 1=yes)
      //  0x18: state (dword, 0=idle, 1=requesting, 7=done)
      //  0x1C: status (dword, 0=OK, 5=failed, 0x715=default)
      //  0x20: task_handle* (bdRemoteTask*)
      //  0x68: socket-related pointer

      SOCKET local_socket (INVALID_SOCKET);

      void
      create_local_socket ()
      {
        local_socket = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (local_socket == INVALID_SOCKET)
          throw runtime_error ("failed to create local udp socket");

        // Bind to localhost on an ephemeral port. We just let the OS pick an
        // appropriate port for us here rather than hardcoding one. If this
        // fails, there is no point in continuing, so we throw.
        //
        sockaddr_in a {};
        a.sin_family = AF_INET;
        a.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
        a.sin_port = 0;

        if (bind (local_socket,
                  reinterpret_cast<sockaddr*> (&a),
                  sizeof (a)) == SOCKET_ERROR)
        {
          closesocket (local_socket);
          local_socket = INVALID_SOCKET;
          throw runtime_error ("failed to bind local udp socket");
        }

        // Make the socket non-blocking so we never accidentally stall the main
        // thread.
        //
        u_long m (1);
        ioctlsocket (local_socket, FIONBIO, &m);
      }

      bool
      bandwidth_test_client_init (void* self)
      {
        auto o (static_cast<uint8_t*> (self));

        // If it's already initialized, we just match the original engine
        // behavior and bail out early.
        //
        if (*reinterpret_cast<int32_t*> (o + 0x14) != 0)
          return false;

        // Make sure we have our local UDP socket ready to back the object.
        //
        if (local_socket == INVALID_SOCKET)
          create_local_socket ();

        // Go ahead and mark it as initialized, setting the status to OK.
        //
        *reinterpret_cast<int32_t*> (o + 0x14) = 1;
        *reinterpret_cast<int32_t*> (o + 0x1C) = 0;

        log::info << "bandwidth test client initialized";

        return true;
      }

      void
      bandwidth_test_client_start (void* self, int controller_index)
      {
        auto o (static_cast<uint8_t*> (self));

        // First, check our preconditions. If we are not initialized, there is
        // nothing to do here.
        //
        if (*reinterpret_cast<int32_t*> (o + 0x14) == 0)
          return;

        // Now, mark the test as successfully completed.
        //
        // Notice that we set the state to 7 (BD_DONE). When the pump switch
        // evaluates this, it naturally falls through to the default case and
        // returns immediately. The Demonware frame handler's state-1 check will
        // then see that status == 0 and safely read the zeroed bandwidth
        // values.
        //
        *reinterpret_cast<int32_t*> (o + 0x18) = 7;
        *reinterpret_cast<int32_t*> (o + 0x1C) = 0;
        *reinterpret_cast<int32_t*> (o + 0x10) = controller_index;
      }
    }

    bandwidth_test_client::
    bandwidth_test_client ()
    {
      detour (bdBandwidthTestClientInit,  &bandwidth_test_client_init);
      detour (bdBandwidthTestClientStart, &bandwidth_test_client_start);
    }
  }
}
