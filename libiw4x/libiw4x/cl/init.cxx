#include <libiw4x/cl/init.hxx>

#include <libiw4x/cl/cl-main.hxx>

namespace iw4x
{
  namespace cl
  {
    bool
    init ()
    {
      ctx.sched.create ("com_frame");

      detour (Com_Frame_Try_Block_Function, &com_frame_try_block_function);

      return true;
    }
  }
}
