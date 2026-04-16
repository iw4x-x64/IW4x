#pragma once

#include <cstdint>
#include <functional>

#include <libiw4x/demonware/core/containers/bit-buffer.hxx>
#include <libiw4x/demonware/lobby/remote-task-manager/remote-task.hxx>

namespace iw4x
{
  namespace demonware
  {
    using service_handler_t =
      std::move_only_function<bool (uint8_t service_id,
                                    uint8_t sub_function_id,
                                    bit_buffer_reader& request,
                                    bit_buffer_writer& reply)>;

    class remote_task_manager
    {
    public:
      remote_task_manager ();

      static void
      register_handler (uint8_t service_id, service_handler_t handler);

      // Create a completed task for the given request.
      //
      static bd_remote_task*
      start_task (uint8_t service_id, uint8_t sub_function_id, void* payload);
    };
  }
}
