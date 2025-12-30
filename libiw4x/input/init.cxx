#include <libiw4x/input/init.hxx>
#include <libiw4x/console/init.hxx>

using namespace std;

namespace iw4x
{
  namespace input
  {
    namespace
    {
      void
      cl_key_event (int localClientNum, int key, bool down, int time)
      {
        static constexpr char tilde (126);
        static constexpr char grave (96);

        if (key == tilde || key == grave)
        {
          Con_ToggleConsole ();
        }

        CL_KeyEvent (localClientNum, key, down, time);
      }
    }

    void
    init ()
    {
      detour (CL_KeyEvent, &cl_key_event);
    }
  }
}
