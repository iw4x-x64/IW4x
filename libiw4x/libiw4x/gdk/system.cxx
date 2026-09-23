#include <libiw4x/gdk/system.hxx>

#include <libiw4x/logger.hxx>

#include <libiw4x/gdk/error.hxx>
#include <libiw4x/gdk/argument.hxx>

using namespace std;

namespace iw4x
{
  namespace gdk
  {
    HRESULT WINAPI xsystem::
    get_xbox_live_sandbox_id (void*,
                              size_t size,
                              char* buffer,
                              size_t* used) noexcept
    {
      return guard ("XSystemGetXboxLiveSandboxId", [&] () -> HRESULT
      {
        copy_out (chars (sandbox, sizeof (sandbox) - 1), size, buffer, used);

        l1 ("XSystemGetXboxLiveSandboxId -> {}", sandbox);

        return S_OK;
      });
    }
  }
}
