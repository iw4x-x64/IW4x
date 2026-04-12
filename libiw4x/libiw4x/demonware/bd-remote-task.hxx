#pragma once

#include <cstdint>

#include <libiw4x/demonware/bd-bit-buffer.hxx>

namespace iw4x
{
  namespace demonware
  {
    // Layout-compatible bdRemoteTask.
    //
    // 0x14012ED70 iterates 32 task slots and calls bdRemoteTask::getStatus() on
    // each.
    //
    // 0x00  next*             linked list pointer (unused, null)
    // 0x08  timeout           float (seconds)
    // 0x0C  status            0=empty, 1=pending, 2=done, 3=failed
    // 0x10  result_buffer*    bd_bit_buffer* (ref-counted)
    // 0x18  request_buffer*   bd_bit_buffer* (ref-counted)
    // 0x20  transaction_id    uint64
    // 0x28  (reserved)
    //
    struct bd_remote_task
    {
      void*          next;
      float          timeout;
      int32_t        status;
      bd_bit_buffer* result_buffer;
      bd_bit_buffer* request_buffer;
      uint64_t       transaction_id;
      uint64_t       reserved;
    };

    static_assert (sizeof (bd_remote_task) == 0x30);

    // Allocate a bd_bit_buffer via the game's allocator and fill it with the
    // serialized contents of a bit_buffer_writer.
    //
    bd_bit_buffer*
    make_bit_buffer (const bit_buffer_writer& src);

    // Allocate a bd_remote_task via the game's allocator in the completed state
    // with the given result buffer attached.
    //
    bd_remote_task*
    make_completed_task (bd_bit_buffer* result, uint64_t transaction_id);
  }
}
