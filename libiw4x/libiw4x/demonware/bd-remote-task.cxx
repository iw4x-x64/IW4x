#include <libiw4x/demonware/bd-remote-task.hxx>

#include <cassert>
#include <cstring>

using namespace std;

namespace iw4x
{
  namespace demonware
  {
    namespace
    {
      // Map what appears to be the native memory allocator used by Demonware.
      // It shares the exact same signature as the standard malloc.
      //
      using alloc_t = void* (*) (size_t);
      auto alloc (reinterpret_cast<alloc_t> (0x140315420));

      // Grab the bdBitBuffer vtable and attach it to every result buffer we
      // create. That is, we want to force the game to performs virtual dispatch
      // on our emulated buffers so that it actually resolves to the correct
      // native functions instead of faulting.
      //
      auto bd_bit_buffer_vtable (reinterpret_cast<void*> (0x1403DA2D0));
    }

    bd_bit_buffer*
    make_bit_buffer (const bit_buffer_writer& s)
    {
      auto b (static_cast<bd_bit_buffer*> (alloc (sizeof (bd_bit_buffer))));
      auto sz (s.size ());
      auto d (static_cast<uint8_t*> (alloc (sz + 1)));

      memcpy (d, s.data (), sz);
      d[sz] = 0;

      // Wire up the fake buffer. Notice we force the type checking flag to
      // true and set the reference count to 1 initially.
      //
      *b = bd_bit_buffer
      {
        .vtable = bd_bit_buffer_vtable,
        .refcount = 1,
        .pad0 = 0,
        .data = d,
        .capacity = static_cast<int32_t> (sz),
        .element_count = static_cast<int32_t> (sz),
        .write_position = static_cast<int32_t> (s.bit_size ()),
        .max_write_position = static_cast<int32_t> (s.bit_size ()),
        .read_position = 0,
        .flags = 0,
        .type_checking = 1,
        .pad1 = 0
      };

      return b;
    }

    bd_remote_task*
    make_completed_task (bd_bit_buffer* r, uint64_t tid)
    {
      auto t (
        static_cast<bd_remote_task*> (alloc (sizeof (bd_remote_task))));

      assert (t != nullptr);

      // Build the task and immediately mark it as done (status 2).
      //
      *t = bd_remote_task
      {
        .next = nullptr,
        .timeout = 0.0f,
        .status = 2, // bd_done
        .result_buffer = r,
        .request_buffer = nullptr,
        .transaction_id = tid,
        .reserved = 0
      };

      // Bump the reference count on the result buffer since it is now
      // conceptually owned by both the task itself and the original caller.
      //
      if (r != nullptr)
        r->refcount = 2;

      return t;
    }
  }
}
