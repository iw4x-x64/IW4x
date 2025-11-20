#include <libiw4x/dedicated/init.hxx>

#include <libiw4x/iw4x-options.hxx>

using namespace std;

namespace iw4x
{
  namespace dedicated
  {
    void
    init ()
    {
      options o (__argc, __argv);

      // Handle --dedicated.
      //
      if (o.dedicated())
      {
        // ...
      }
    }
  }
}
