#include <libiw4x/demonware/bd-remote-task-manager.hxx>

#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <mutex>

#include <libiw4x/detour.hxx>
#include <libiw4x/logger.hxx>

using namespace std;

namespace iw4x
{
  namespace demonware
  {
    namespace
    {
      // Mutex and registry for service handlers.
      //
      mutex handlers_m;
      map<uint8_t, service_handler_t> handlers;

      // Transaction ID counter. We use this to sequence our replies back to
      // the caller.
      //
      uint64_t next_tid (1);

      // bdLobbyConnection::startTask (0x140322CC0).
      //
      // Note that the idea here is to intercept the call, route it through our
      // remote task manager, and hand back a task that is already marked as
      // done. Perhaps we should move it to the lobby service later.
      //
      using startTask_t =
        int32_t (*) (void*, void**, uint8_t, uint8_t, void*, float);
      startTask_t startTask (reinterpret_cast<startTask_t> (0x140322CC0));

      [[gnu::ms_abi]] int32_t
      start_task (void*   connection,
                  void**  task_out,
                  uint8_t service_id,
                  uint8_t sub_func_id,
                  void*   payload,
                  float   timeout)
      {
        auto task (
          remote_task_manager::start_task (service_id, sub_func_id, payload));

        // See if the caller expects a task object back.
        //
        if (task_out != nullptr)
          *task_out = task;

        return 0; // BD_NO_ERROR.
      }
    }

    remote_task_manager::
    remote_task_manager ()
    {
      // Detour the original function.
      //
      detour (startTask, &start_task);
    }

    void remote_task_manager::
    register_handler (uint8_t service_id, service_handler_t h)
    {
      lock_guard<mutex> lk (handlers_m);
      handlers[service_id] = move (h);
    }

    bd_remote_task* remote_task_manager::
    start_task (uint8_t service_id,
                uint8_t sub_function_id,
                void*   payload)
    {
      // We need to extract the underlying raw bytes from the game's bdBitBuffer
      // object. Since we receive this as an opaque payload and don't want to
      // rely on calling the game's native member functions, we unpack the state
      // directly using known memory offsets.
      //
      // The memory layout of the object looks like this:
      //
      // 0x00: vtable
      // 0x10: uint8_t* (pointer to the actual allocated buffer)
      // 0x20: int32_t  (current write position in bits)
      // 0x2D: uint8_t  (boolean flag indicating if type tags are used)

      // Set up safe defaults. If the payload is missing or empty, we will just
      // point to a dummy byte. This prevents us from passing a null pointer
      // to our reader later on, keeping the downstream logic simple.
      //
      static const uint8_t empty_byte (0);

      const uint8_t* req_data (&empty_byte);
      size_t req_size (0);
      bool use_types (false);

      // See if we actually got a payload to unpack.
      //
      if (payload != nullptr)
      {
        auto buf (static_cast<uint8_t*> (payload));

        // Extract the internal fields. Notice that the buffer size is tracked
        // in bits, so we must round up to the nearest byte to get the actual
        // size in memory.
        //
        auto data (*reinterpret_cast<uint8_t**> (buf + 0x10));
        auto write_bits (*reinterpret_cast<int32_t*> (buf + 0x20));
        auto data_size (static_cast<size_t> ((write_bits + 7) / 8));

        use_types = (buf[0x2D] != 0);

        // Finally, make sure the internal buffer is actually allocated and has
        // data before we overwrite our safe defaults.
        //
        if (data != nullptr && data_size > 0)
        {
          req_data = data;
          req_size = data_size;
        }
      }

      // Build a reader over the raw request data. Note that the native
      // bdBitBuffer constructor slaps a 1-bit header at position 0 to indicate
      // if type checking is enabled. We skip it so our reads correctly align
      // with the first real type tag.
      //
      bit_buffer_reader req (req_data, req_size);
      req.set_position (use_types ? 1 : 0);

      // Now dispatch to whoever is registered to handle this.
      //
      bit_buffer_writer rep;
      bool done (false);

      {
        lock_guard<mutex> lk (handlers_m);

        auto i (handlers.find (service_id));

        // See if we actually have a handler.
        //
        if (i != handlers.end ())
          done = i->second (service_id, sub_function_id, req, rep);
      }

      // If nobody wrote a reply, then we just fake a generic success response.
      //
      if (rep.size () == 0)
      {
        rep.write_uint32 (0); // BD_NO_ERROR.
        rep.write_uint8 (0);  // No results.
      }

      // Finally, stitch together the completed task.
      //
      auto id (next_tid++);
      auto res (make_bit_buffer (rep));

      return make_completed_task (res, id);
    }
  }
}
