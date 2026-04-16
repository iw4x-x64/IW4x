#include <libiw4x/demonware/lobby/remote-task-manager/remote-task.hxx>

#include <cassert>
#include <cstring>

#include <libiw4x/import.hxx>

using namespace std;

namespace iw4x
{
  namespace demonware
  {
    bd_remote_task*
    make_completed_task (bd_bit_buffer* r, uint64_t id)
    {
      auto t (static_cast<bd_remote_task*> (bdAlloc (sizeof (bd_remote_task))));

      *t = bd_remote_task
      {
        .next           = nullptr,
        .timeout        = 0.0f,
        .status         = 2, // bd_done
        .result_buffer  = r,
        .request_buffer = nullptr,
        .transaction_id = id,
        .reserved       = 0
      };

      // Bump the reference count on the result buffer. That is, it's now
      // conceptually owned by both the task itself and the caller, so it has to
      // survive until both are finished with it.
      //
      if (r != nullptr)
        r->refcount = 2;

      return t;
    }
  }
}
