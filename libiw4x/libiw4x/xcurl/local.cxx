#include <libiw4x/xcurl/local.hxx>

#include <algorithm>

#include <libiw4x/logger.hxx>

#include <libiw4x/gdk/sync.hxx>

using namespace std;

namespace iw4x
{
  namespace xcurl
  {
    using namespace gdk;

    string_view
    parameter (string_view url, string_view name) noexcept
    {
      size_t q (url.find ('?'));

      if (q == string_view::npos)
      {
        return string_view ();
      }

      url.remove_prefix (q + 1);

      while (!url.empty ())
      {
        size_t e (url.find ('&'));
        string_view p (url.substr (0, e));
        size_t v (p.find ('='));

        if (v != string_view::npos && p.substr (0, v) == name)
        {
          p.remove_prefix (v + 1);
          return p;
        }

        if (e == string_view::npos)
        {
          break;
        }

        url.remove_prefix (e + 1);
      }

      return string_view ();
    }

    text<url_limit>
    redirect (string_view url, const destination& d) noexcept
    {
      size_t a (url.find ("://"));

      if (a != string_view::npos)
      {
        url.remove_prefix (a + 3);
      }

      auto p (ranges::find_if (url, [] (char c)
      {
        return c == '/' || c == '?' || c == '#';
      }));

      return text<url_limit> ("http://{}:{}{}",
                              d.host,
                              d.port,
                              string_view (p, url.end ()));
    }

    namespace
    {
      bool
      privacy (string_view url, response& r) noexcept
      {
        string_view setting (parameter (url, "setting"));
        string_view target (parameter (url, "target"));

        l1 ("privacy: {} of {} allowed",
            !setting.empty () ? setting : "<none>",
            !target.empty () ? target : "<none>");

        r.status = 200;
        r.type = text<type_limit> ("application/json");
        r.body = text<body_limit> ("{\"isAllowed\":true}");

        return true;
      }

      struct endpoint
      {
        const char* method;
        const char* host;
        const char* path;

        bool (*serve) (string_view url, response&) noexcept;
      };

      constexpr endpoint endpoints[]
      {
        {"GET", "privacy.xboxlive.com", "/permission/validate", &privacy}
      };

      const endpoint*
      lookup (string_view method, string_view url) noexcept
      {
        const endpoint* e (ranges::find_if (endpoints, [&] (const endpoint& x)
        {
          return method == x.method &&
                 url.contains (x.host) &&
                 url.contains (x.path);
        }));

        return e != ranges::end (endpoints) ? e : nullptr;
      }

      struct platform_endpoint
      {
        mutex            mutex_;
        text<host_limit> host;
        int              port = 0;
      };

      platform_endpoint&
      installed () noexcept
      {
        static platform_endpoint p;
        return p;
      }

      bool
      redirected (string_view url) noexcept
      {
        if (url.contains ("social.xboxlive.com") &&
            url.contains ("/people"))
          return true;

        return url.contains ("multiplayeractivity.xboxlive.com");
      }
    }

    void
    serve_platform_at (string_view host, int port) noexcept
    {
      platform_endpoint& p (installed ());
      scope_lock         l (p.mutex_);

      p.host = text<host_limit> ("{}", host);
      p.port = port;

      info ("the IW4x platform is at {}:{}", p.host.c_str (), port);
    }

    bool
    answer (string_view method, string_view url, response& r) noexcept
    {
      const endpoint* e (lookup (method, url));

      return e != nullptr && e->serve (url, r);
    }

    bool
    answered (string_view method, string_view url) noexcept
    {
      return lookup (method, url) != nullptr;
    }

    bool
    platform (string_view url, destination& d) noexcept
    {
      if (!redirected (url))
        return false;

      platform_endpoint& p (installed ());
      scope_lock         l (p.mutex_);

      if (p.port == 0)
      {
        warn ("{} is the platform's to answer and none is installed", url);

        return false;
      }

      d.host = p.host;
      d.port = p.port;

      return true;
    }

    bool
    served (string_view method, string_view url) noexcept
    {
      destination d;

      return answered (method, url) || platform (url, d);
    }
  }
}
