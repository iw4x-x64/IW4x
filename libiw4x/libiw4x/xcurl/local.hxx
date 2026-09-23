#pragma once

#include <cstddef>

#include <libiw4x/gdk/text.hxx>

#include <libiw4x/xcurl/xcurl.hxx>

namespace iw4x
{
  namespace xcurl
  {
    inline constexpr std::size_t body_limit (1024);
    inline constexpr std::size_t type_limit (64);
    inline constexpr std::size_t host_limit (256);

    struct response
    {
      long                  status = 0;
      gdk::text<type_limit> type;
      gdk::text<body_limit> body;
    };

    struct destination
    {
      gdk::text<host_limit> host;
      int                   port = 0;
    };

    bool
    answer (gdk::chars method, gdk::chars url, response&) noexcept;

    bool
    answered (gdk::chars method, gdk::chars url) noexcept;

    bool
    platform (gdk::chars url, destination&) noexcept;

    bool
    served (gdk::chars method, gdk::chars url) noexcept;

    void
    serve_platform_at (gdk::chars host, int port) noexcept;

    gdk::text<url_limit>
    redirect (gdk::chars url, const destination&) noexcept;

    gdk::chars
    parameter (gdk::chars url, gdk::chars name) noexcept;

    bool
    contains (gdk::chars haystack, gdk::chars needle) noexcept;
  }
}
