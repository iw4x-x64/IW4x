#include <libiw4x/demonware/bd-bandwidth-test-client.hxx>

#include <cstdint>
#include <cstring>

#include <winsock2.h>

#include <libiw4x/detour.hxx>
#include <libiw4x/logger.hxx>

using namespace std;

namespace iw4x
{
  namespace demonware
  {
    namespace
    {
      // The bdBandwidthTestClient object layout
      //
      //  0x00: vtable*
      //  0x10: controller_index (dword)
      //  0x14: initialized (dword, 0=no, 1=yes)
      //  0x18: state (dword, 0=idle, 1=requesting, 7=done)
      //  0x1C: status (dword, 0=OK, 5=failed, 0x715=default)
      //  0x20: task_handle* (bdRemoteTask*)
      //  0x68: socket-related pointer

      // We need a local UDP socket created for the bandwidth test client. Doing
      // this gives the object a real, valid socket handle so that any code path
      // down the line that checks for socket validity won't blow up.
      //
      SOCKET local_socket (INVALID_SOCKET);

      void
      create_local_socket ()
      {
        local_socket = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (local_socket == INVALID_SOCKET)
        {
          log::warning << "dw: bandwidth: failed to create local UDP socket";
          return;
        }

        // Bind to localhost on an ephemeral port. We just let the OS pick
        // an appropriate port for us here.
        //
        sockaddr_in addr {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
        addr.sin_port = 0;

        if (bind (local_socket,
                  reinterpret_cast<sockaddr*> (&addr),
                  sizeof (addr)) == SOCKET_ERROR)
        {
          log::warning << "dw: bandwidth: failed to bind local UDP socket";
          closesocket (local_socket);
          local_socket = INVALID_SOCKET;
          return;
        }

        // Make it non-blocking so we never stall the game thread.
        //
        u_long mode (1);
        ioctlsocket (local_socket, FIONBIO, &mode);

        log::debug << "dw: bandwidth: local UDP socket created";
      }

      // bdBandwidthTestClient::init (0x140321560).
      //
      // The original implementation creates a UDP socket via vtable[1], and
      // then sets the initialized flag along with the status. We don't really
      // want to do all that, so we skip the real socket creation and just
      // back it with our local socket instead.
      //
      using bdBandwidthTestClientInit_t = bool (*) (void*);
      bdBandwidthTestClientInit_t bdBandwidthTestClientInit (
        reinterpret_cast<bdBandwidthTestClientInit_t> (0x140321560));

      bool __attribute__ ((ms_abi))
      bandwidth_test_client_init (void* self)
      {
        auto obj (static_cast<uint8_t*> (self));

        // If it is already initialized, we just match the original
        // behavior and bail out early (returning false).
        //
        if (*reinterpret_cast<int32_t*> (obj + 0x14) != 0)
          return false;

        // Make sure we have our local UDP socket ready to back the object.
        //
        if (local_socket == INVALID_SOCKET)
          create_local_socket ();

        // Go ahead and mark it as initialized, setting the status to OK.
        //
        *reinterpret_cast<int32_t*> (obj + 0x14) = 1;
        *reinterpret_cast<int32_t*> (obj + 0x1C) = 0;

        log::info << "dw: bandwidth: init (local socket)";

        return true;
      }

      // bdBandwidthTestClient::start (0x140322260).
      //
      // Normally, this gets the lobby connection, serializes some parameters,
      // creates a bdRemoteTask, and finally sends a bandwidth test request
      // through the connection. We skip all of that and immediately report the
      // test as completed with default bandwidth values.
      //
      using bdBandwidthTestClientStart_t = void (*) (void*, int);
      bdBandwidthTestClientStart_t bdBandwidthTestClientStart (
        reinterpret_cast<bdBandwidthTestClientStart_t> (0x140322260));

      void __attribute__ ((ms_abi))
      bandwidth_test_client_start (void* self, int controller_index)
      {
        auto obj (static_cast<uint8_t*> (self));

        // First, check our preconditions. If we are not initialized, there is
        // nothing to do.
        //
        if (*reinterpret_cast<int32_t*> (obj + 0x14) == 0)
          return;

        // Now, mark the test as successfully completed.
        //
        // Notice that we set state = 7 (BD_DONE). When the pump switch
        // evaluates this, it falls to the default case and returns
        // immediately. The Demonware frame handler's state-1 check will
        // see that status == 0 and read the (zero) bandwidth values.
        //
        *reinterpret_cast<int32_t*> (obj + 0x18) = 7;
        *reinterpret_cast<int32_t*> (obj + 0x1C) = 0;
        *reinterpret_cast<int32_t*> (obj + 0x10) = controller_index;

        log::info << "dw: bandwidth: start (immediate completion, controller="
                  << controller_index << ")";
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
