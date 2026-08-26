#include <libiw4x/gdk/user.hxx>
#include <libiw4x/gdk/async.hxx>
#include <libiw4x/gdk/task-queue.hxx>

#include <libiw4x/xcurl/local.hxx>
#include <libiw4x/xcurl/runtime.hxx>

#undef NDEBUG
#include <cassert>

using namespace iw4x;
using namespace iw4x::gdk;

namespace
{
  const char privacy_url[] =
    "https://privacy.xboxlive.com/users/xuid(9)/permission/validate"
    "?setting=Communications&target=xuid(7)";

  const char social_url[] =
    "https://social.xboxlive.com/users/me/people";

  const char microsoft_url[] =
    "https://profile.xboxlive.com/users/me/profile/settings";

  task_queue* queue (nullptr);

  HRESULT
  token_for (void* user, const char* url)
  {
    async_block b {};
    b.queue = queue;

    return xuser::get_token_and_signature_async (nullptr,
                                                 user,
                                                 0,
                                                 "GET",
                                                 url,
                                                 0, nullptr, 0, nullptr,
                                                 &b);
  }
}

int
main ()
{
  queue = create_queue (dispatch_mode::manual, dispatch_mode::manual);
  assert (queue != nullptr);

  set_process_queue (queue);

  void* user (nullptr);

  {
    async_block b {};
    b.queue = queue;

    assert (xuser::add_async (nullptr, 0, &b) == S_OK);

    while ((*queue)[port::work].dispatch (0))
      ;

    assert (xuser::add_result (nullptr, &b, &user) == S_OK);
  }

  assert (token_for (user, privacy_url) == E_NOTIMPL);
  assert (token_for (user, social_url) == E_NOTIMPL);
  assert (token_for (user, microsoft_url) == E_NOTIMPL);

  xcurl::install ();
  xcurl::serve_platform_at (xcurl::chars ("platform.iw4x"), 3074);

  assert (token_for (user, privacy_url) == S_OK);
  assert (token_for (user, social_url) == S_OK);

  assert (token_for (user, microsoft_url) == E_NOTIMPL);

  assert (xcurl::served (chars ("GET"), chars (privacy_url)));
  assert (xcurl::served (chars ("GET"), chars (social_url)));
  assert (!xcurl::served (chars ("GET"), chars (microsoft_url)));

  while ((*queue)[port::work].dispatch (0))
    ;

  stop_queues ();
}
