#include <libiw4x/gdk/argument.hxx>

using namespace std;

namespace iw4x
{
  namespace gdk
  {
    void
    copy_out (chars v, size_t size, char* buffer, size_t* used)
    {
      if (buffer == nullptr)
        raise (E_POINTER, "no buffer");

      size_t n (v.size () + 1);

      if (size < n)
        raise (insufficient_buffer,
               "a {} byte buffer holds none of a {} byte answer",
               size,
               n);

      __builtin_memcpy (buffer, v.data (), v.size ());

      buffer[v.size ()] = '\0';

      if (used != nullptr)
        *used = n;
    }
  }
}
