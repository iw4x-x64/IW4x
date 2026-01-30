#include <libiw4x/client/init.hxx>

#include <libiw4x/client/common.hxx>

namespace iw4x
{
  namespace client
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
