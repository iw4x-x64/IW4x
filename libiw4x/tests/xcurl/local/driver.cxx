#include <cstring>

#include <libiw4x/xcurl/local.hxx>

#undef NDEBUG
#include <cassert>

using namespace iw4x;
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
  assert (contains (chars ("abcdef"), chars ("abc")));
  assert (contains (chars ("abcdef"), chars ("cde")));
  assert (contains (chars ("abcdef"), chars ("def")));
  assert (contains (chars ("abcdef"), chars ("")));

  assert (!contains (chars ("abcdef"), chars ("xyz")));
  assert (!contains (chars ("abc"), chars ("abcd")));
  assert (!contains (chars (""), chars ("a")));

  auto value = [] (const char* u, const char* n)
  {
    return parameter (chars (u), chars (n));
  };

  assert (value (privacy_url, "setting") == chars ("Communications"));
  assert (value (privacy_url, "target") == chars ("xuid(7)"));
  assert (value (privacy_url, "missing").empty ());

  assert (value ("http://h/p", "a").empty ());
  assert (value ("http://h/p?a=1", "a") == chars ("1"));
  assert (value ("http://h/p?a=1&b=2", "b") == chars ("2"));
  assert (value ("http://h/p?ab=1", "a").empty ());
  assert (value ("http://h/p?a", "a").empty ());

  auto to = [] (const char* u, const char* e)
  {
    destination d;

    d.host = text<host_limit> ("platform.iw4x");
    d.port = 8080;

    return std::strcmp (redirect (chars (u), d).c_str (), e) == 0;
  };

  assert (to ("https://social.xboxlive.com/users/me/people",
              "http://platform.iw4x:8080/users/me/people"));
  assert (to ("https://h.com:443/p?q=1", "http://platform.iw4x:8080/p?q=1"));
  assert (to ("https://h.com", "http://platform.iw4x:8080"));
  assert (to ("https://h.com/", "http://platform.iw4x:8080/"));
  assert (to ("https://h.com/p#f", "http://platform.iw4x:8080/p#f"));

  {
    response r;

    assert (answer (chars ("GET"), chars (privacy_url), r));

    assert (r.status == 200);
    assert (std::strcmp (r.type.c_str (), "application/json") == 0);
    assert (std::strcmp (r.body.c_str (), "{\"isAllowed\":true}") == 0);
  }

  assert (answered (chars ("GET"), chars (privacy_url)));
  assert (served (chars ("GET"), chars (privacy_url)));

  assert (!answered (chars ("POST"), chars (privacy_url)));
  assert (!answered (chars ("GET"),
                     chars ("https://privacy.xboxlive.com/users/xuid(9)"
                            "/people/avoid")));
  assert (!answered (chars ("GET"), chars ("https://example.com/x")));
  assert (!served (chars ("GET"), chars ("https://example.com/x")));

  {
    destination d;

    assert (!platform (chars ("https://social.xboxlive.com/users/me/people"),
                       d));

    serve_platform_at (chars ("platform.iw4x"), 3074);

    assert (platform (chars ("https://social.xboxlive.com/users/me/people"),
                      d));
    assert (std::strcmp (d.host.c_str (), "platform.iw4x") == 0);
    assert (d.port == 3074);

    assert (platform (chars ("https://multiplayeractivity.xboxlive.com/x"),
                      d));

    assert (!platform (chars ("https://social.xboxlive.com/users/me/other"),
                       d));
    assert (!platform (chars ("https://example.com/people"), d));

    assert (served (chars ("GET"),
                    chars ("https://social.xboxlive.com/users/me/people")));
  }
}
