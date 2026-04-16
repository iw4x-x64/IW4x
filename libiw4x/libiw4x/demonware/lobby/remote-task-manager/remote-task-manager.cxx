#include <libiw4x/demonware/lobby/remote-task-manager/remote-task-manager.hxx>

#include <cstdint>
#include <cstring>
#include <map>
#include <mutex>

#include <libiw4x/detour.hxx>
#include <libiw4x/import.hxx>
#include <libiw4x/logger.hxx>

#include <libiw4x/demonware/lobby/connection.hxx>

using namespace std;

namespace iw4x
{
  namespace demonware
  {
    namespace
    {
      // Service handlers registry, along with a mutex to protect it from
      // concurrent access during registration and dispatch.
      //
      mutex handlers_m;
      map<uint8_t, service_handler_t> handlers;

      // Transaction ID counter.
      //
      // We use this to sequence our replies back to the caller. Note that
      // we start with 1 to avoid treating 0 as a valid transaction ID in
      // some edge cases.
      //
      uint64_t next_tid (1);
    }

    void remote_task_manager::
    register_handler (uint8_t service_id,
                      service_handler_t h)
    {
      lock_guard<mutex> lk (handlers_m);
      handlers[service_id] = move (h);
    }

    bd_remote_task* remote_task_manager::
    start_task (uint8_t service_id,
                uint8_t sub_function_id,
                void*   payload)
    {
      // We need to extract the underlying raw bytes from the game's
      // bdBitBuffer object. Since we receive this as an opaque payload and
      // don't want to rely on calling the game's native member functions, we
      // unpack the state directly using known memory offsets.
      //
      // The memory layout of the object looks like this:
      //
      // 0x00: vtable
      // 0x10: uint8_t* (pointer to the actual allocated buffer)
      // 0x20: int32_t  (current write position in bits)
      // 0x2D: uint8_t  (boolean flag indicating if type tags are used)

      // Figure out if we actually got a payload to unpack and extract the
      // internal fields.
      //
      // Notice that the buffer size is tracked in bits, so we must round up
      // to the nearest byte to get the actual size in memory.
      //
      auto buf (static_cast<uint8_t*> (payload));

      auto data (buf != nullptr
                 ? *reinterpret_cast<uint8_t**> (buf + 0x10)
                 : nullptr);

      auto write_bits (buf != nullptr
                       ? *reinterpret_cast<int32_t*> (buf + 0x20)
                       : 0);

      auto data_size (static_cast<size_t> ((write_bits + 7) / 8));

      // Set up safe defaults.
      //
      // If the payload is missing or empty, we will just point to a dummy
      // byte.
      //
      static const uint8_t empty_byte (0);

      const uint8_t* req_data (data != nullptr && data_size > 0
                               ? data
                               : &empty_byte);

      size_t req_size (data != nullptr && data_size > 0
                       ? data_size
                       : 0);

      bool use_types (buf != nullptr && buf[0x2D] != 0);

      // Build a reader over the raw request data.
      //
      // Note that the native bdBitBuffer constructor slaps a 1-bit header
      // at position 0 to indicate if type checking is enabled. We skip it
      // so our reads correctly align with the first real type tag.
      //
      bit_buffer_reader req (req_data, req_size);
      req.set_position (use_types ? 1 : 0);

      // Now dispatch to whoever is registered to handle this.
      //
      bit_buffer_writer rep;

      {
        lock_guard<mutex> lk (handlers_m);
        auto i (handlers.find (service_id));

        // Check if we actually have a handler for this service and let it
        // populate our reply buffer.
        //
        if (i != handlers.end ())
          i->second (service_id, sub_function_id, req, rep);
      }

      // If the handler didn't write a reply, or if we didn't find a handler
      // in the first place, we just fake a generic success response.
      //
      if (rep.size () == 0)
      {
        rep.write_uint32 (0); // BD_NO_ERROR.
        rep.write_uint8 (0);  // No results.
      }

      // Finally, stitch together the completed task with a new sequence ID
      // and our prepared reply buffer.
      //
      auto id (next_tid++);
      auto res (make_bit_buffer (rep));

      return make_completed_task (res, id);
    }

    remote_task_manager::
    remote_task_manager ()
    {
      detour (bdLobbyConnectionStartTask, &lobby_connection_start_task);
    }
  }
}
