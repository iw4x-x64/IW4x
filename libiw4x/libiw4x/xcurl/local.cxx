#include <libiw4x/xcurl/local.hxx>

#include <libiw4x/logger.hxx>

namespace iw4x
{
  namespace xcurl
  {
    bool
    contains (chars h, chars n) noexcept
    {
      if (n.size () > h.size ())
        return false;

      for (std::size_t i (0); i + n.size () <= h.size (); ++i)
      {
        if (__builtin_memcmp (h.data () + i, n.data (), n.size ()) == 0)
          return true;
      }

      return false;
    }

    chars
    parameter (chars url, chars name) noexcept
    {
      std::size_t q (0);

      for (; q != url.size () && url.data ()[q] != '?'; ++q)
        ;

      if (q == url.size ())
        return chars ();

      for (std::size_t i (q + 1); i < url.size ();)
      {
        std::size_t e (i);

        for (; e != url.size () && url.data ()[e] != '&'; ++e)
          ;

        std::size_t v (i);

        for (; v != e && url.data ()[v] != '='; ++v)
          ;

        if (v != e &&
            v - i == name.size () &&
            __builtin_memcmp (url.data () + i, name.data (), name.size ()) == 0)
          return chars (url.data () + v + 1, e - v - 1);

        i = e + 1;
      }

      return chars ();
    }

    text<url_limit>
    redirect (chars url, const destination& d) noexcept
    {
      std::size_t a (0);

      for (; a + 3 <= url.size (); ++a)
      {
        if (__builtin_memcmp (url.data () + a, "://", 3) == 0)
        {
          a += 3;
          break;
        }
      }

      if (a + 3 > url.size ())
        a = 0;

      std::size_t p (a);

      for (; p != url.size (); ++p)
      {
        char c (url.data ()[p]);

        if (c == '/' || c == '?' || c == '#')
          break;
      }

      return text<url_limit> ("http://{}:{}{}",
                              d.host,
                              d.port,
                              chars (url.data () + p, url.size () - p));
    }

    namespace
    {
      bool
      privacy (chars url, response& r) noexcept
      {
        chars setting (parameter (url, chars ("setting")));
        chars target (parameter (url, chars ("target")));

        l1 ("privacy: {} of {} allowed",
            setting.size () != 0 ? setting.data () : "<none>",
            target.size () != 0 ? target.data () : "<none>");

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

        bool (*serve) (chars url, response&) noexcept;
      };

      constexpr endpoint endpoints[]
      {
        {"GET", "privacy.xboxlive.com", "/permission/validate", &privacy}
      };

      const endpoint*
      lookup (chars method, chars url) noexcept
      {
#pragma GCC unroll 8
        for (const endpoint& e: endpoints)
        {
          if (method == chars (e.method) &&
              contains (url, chars (e.host)) &&
              contains (url, chars (e.path)))
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
      redirected (chars url) noexcept
      {
        if (contains (url, chars ("social.xboxlive.com")) &&
            contains (url, chars ("/people")))
          return true;

        return contains (url, chars ("multiplayeractivity.xboxlive.com"));
      }
    }

    void
    serve_platform_at (chars host, int port) noexcept
    {
      platform_endpoint& p (installed ());
      scope_lock         l (p.mutex_);

      p.host = text<host_limit> ("{}", host);
      p.port = port;

      info ("the IW4x platform is at {}:{}", p.host.c_str (), port);
    }

    bool
    answer (chars method, chars url, response& r) noexcept
    {
      const endpoint* e (lookup (method, url));

      return e != nullptr && e->serve (url, r);
    }

    bool
    answered (chars method, chars url) noexcept
    {
      return lookup (method, url) != nullptr;
    }

    bool
    platform (chars url, destination& d) noexcept
    {
      if (!redirected (url))
        return false;

      platform_endpoint& p (installed ());
      scope_lock         l (p.mutex_);

      if (p.port == 0)
      {
        warn ("{} is the platform's to answer and none is installed",
              url.data ());

        return false;
      }

      d.host = p.host;
      d.port = p.port;

      return true;
    }

    bool
    served (chars method, chars url) noexcept
    {
      destination d;

      return answered (method, url) || platform (url, d);
    }
  }
}
