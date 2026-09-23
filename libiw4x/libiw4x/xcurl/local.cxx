#include <libiw4x/xcurl/local.hxx>

#include <libiw4x/logger.hxx>

#include <libiw4x/gdk/sync.hxx>

using namespace std;

namespace iw4x
{
  namespace xcurl
  {
    using namespace gdk;

    bool
    contains (string_view h, string_view n) noexcept
    {
      if (n.size () > h.size ())
        return false;

      for (size_t i (0); i + n.size () <= h.size (); ++i)
      {
        if (__builtin_memcmp (h.data () + i, n.data (), n.size ()) == 0)
          return true;
      }

      return false;
    }

    string_view
    parameter (string_view url, string_view name) noexcept
    {
      size_t q (0);

      for (; q != url.size () && url.data ()[q] != '?'; ++q)
        ;

      if (q == url.size ())
        return string_view ();

      for (size_t i (q + 1); i < url.size ();)
      {
        size_t e (i);

        for (; e != url.size () && url.data ()[e] != '&'; ++e)
          ;

        size_t v (i);

        for (; v != e && url.data ()[v] != '='; ++v)
          ;

        if (v != e &&
            v - i == name.size () &&
            __builtin_memcmp (url.data () + i, name.data (), name.size ()) == 0)
          return string_view (url.data () + v + 1, e - v - 1);

        i = e + 1;
      }

      return string_view ();
    }

    text<url_limit>
    redirect (string_view url, const destination& d) noexcept
    {
      size_t a (0);

      for (; a + 3 <= url.size (); ++a)
      {
        if (__builtin_memcmp (url.data () + a, "://", 3) == 0)
        {
          break;
        }
      }

      a = a + 3 <= url.size () ? a + 3 : 0;

      size_t p (a);

      for (; p != url.size (); ++p)
      {
        char c (url.data ()[p]);

        if (c == '/' || c == '?' || c == '#')
          break;
      }

      return text<url_limit> ("http://{}:{}{}",
                              d.host,
                              d.port,
                              string_view (url.data () + p, url.size () - p));
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
#pragma GCC unroll 8
        for (const endpoint& e: endpoints)
        {
          if (method == e.method &&
              contains (url, e.host) &&
              contains (url, e.path))
            return &e;
        }

        return nullptr;
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
        if (contains (url, "social.xboxlive.com") &&
            contains (url, "/people"))
          return true;

        return contains (url, "multiplayeractivity.xboxlive.com");
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
