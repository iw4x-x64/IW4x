#include <cstring>

#include <libiw4x/xcurl/local.hxx>

#undef NDEBUG
#include <cassert>

using namespace iw4x;
using namespace iw4x::gdk;
using namespace iw4x::xcurl;

namespace
{
  const char privacy_url[] =
    "https://privacy.xboxlive.com/users/xuid(9)/permission/validate"
    "?setting=Communications&target=xuid(7)";
}

int
main ()
{
  assert (contains ("abcdef", "abc"));
  assert (contains ("abcdef", "cde"));
  assert (contains ("abcdef", "def"));
  assert (contains ("abcdef", ""));

  assert (!contains ("abcdef", "xyz"));
  assert (!contains ("abc", "abcd"));
  assert (!contains ("", "a"));

  auto value = [] (const char* u, const char* n)
  {
    return parameter (u, n);
  };

  assert (value (privacy_url, "setting") == "Communications");
  assert (value (privacy_url, "target") == "xuid(7)");
  assert (value (privacy_url, "missing").empty ());

  assert (value ("http://h/p", "a").empty ());
  assert (value ("http://h/p?a=1", "a") == "1");
  assert (value ("http://h/p?a=1&b=2", "b") == "2");
  assert (value ("http://h/p?ab=1", "a").empty ());
  assert (value ("http://h/p?a", "a").empty ());

  auto to = [] (const char* u, const char* e)
  {
    destination d;

    d.host = text<host_limit> ("platform.iw4x");
    d.port = 8080;

    return std::strcmp (redirect (u, d).c_str (), e) == 0;
  };

  assert (to ("https://social.xboxlive.com/users/me/people",
              "http://platform.iw4x:8080/users/me/people"));
  assert (to ("https://h.com:443/p?q=1", "http://platform.iw4x:8080/p?q=1"));
  assert (to ("https://h.com", "http://platform.iw4x:8080"));
  assert (to ("https://h.com/", "http://platform.iw4x:8080/"));
  assert (to ("https://h.com/p#f", "http://platform.iw4x:8080/p#f"));
  assert (to ("https://h/p", "http://platform.iw4x:8080/p"));
  assert (to ("https://h", "http://platform.iw4x:8080"));

  {
    response r;

    assert (answer ("GET", privacy_url, r));

    assert (r.status == 200);
    assert (std::strcmp (r.type.c_str (), "application/json") == 0);
    assert (std::strcmp (r.body.c_str (), "{\"isAllowed\":true}") == 0);
  }

  assert (answered ("GET", privacy_url));
  assert (served ("GET", privacy_url));

  assert (!answered ("POST", privacy_url));
  assert (!answered ("GET",
                     "https://privacy.xboxlive.com/users/xuid(9)"
                     "/people/avoid"));
  assert (!answered ("GET", "https://example.com/x"));
  assert (!served ("GET", "https://example.com/x"));

  {
    destination d;

    assert (!platform ("https://social.xboxlive.com/users/me/people", d));

    serve_platform_at ("platform.iw4x", 3074);

    assert (platform ("https://social.xboxlive.com/users/me/people", d));
    assert (std::strcmp (d.host.c_str (), "platform.iw4x") == 0);
    assert (d.port == 3074);

    assert (platform ("https://multiplayeractivity.xboxlive.com/x", d));

    assert (!platform ("https://social.xboxlive.com/users/me/other", d));
    assert (!platform ("https://example.com/people", d));

    assert (served ("GET", "https://social.xboxlive.com/users/me/people"));
  }
}
