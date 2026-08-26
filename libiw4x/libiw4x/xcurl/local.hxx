#pragma once

#include <cstddef>

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
      long                 status = 0;
      text<type_limit>     type;
      text<body_limit>     body;
    };

    struct destination
    {
      text<host_limit> host;
      int              port = 0;
    };

    bool
    answer (chars method, chars url, response&) noexcept;

    bool
    answered (chars method, chars url) noexcept;

    bool
    platform (chars url, destination&) noexcept;

    bool
    served (chars method, chars url) noexcept;

    void
    serve_platform_at (chars host, int port) noexcept;

    text<url_limit>
    redirect (chars url, const destination&) noexcept;

    chars
    parameter (chars url, chars name) noexcept;

    bool
    contains (chars haystack, chars needle) noexcept;
  }
}
