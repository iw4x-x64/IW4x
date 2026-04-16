#pragma once

#include <cstdint>

#include <libiw4x/demonware/core/containers/bit-buffer.hxx>

namespace iw4x
{
  namespace demonware
  {
    // bdRemoteTask compatible object layout.
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

    bd_remote_task*
    make_completed_task (bd_bit_buffer*, uint64_t transaction_id);
  }
}
