#pragma once

#include <cstddef>
#include <string_view>

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
    answer (std::string_view method, std::string_view url, response&) noexcept;

    bool
    answered (std::string_view method, std::string_view url) noexcept;

    bool
    platform (std::string_view url, destination&) noexcept;

    bool
    served (std::string_view method, std::string_view url) noexcept;

    void
    serve_platform_at (std::string_view host, int port) noexcept;

    gdk::text<url_limit>
    redirect (std::string_view url, const destination&) noexcept;

    std::string_view
    parameter (std::string_view url, std::string_view name) noexcept;

    bool
    contains (std::string_view haystack, std::string_view needle) noexcept;
  }
}
