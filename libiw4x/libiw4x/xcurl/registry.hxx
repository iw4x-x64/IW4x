#pragma once

#include <cstdint>

#include <libiw4x/xcurl/multi.hxx>
#include <libiw4x/xcurl/transfer.hxx>

namespace iw4x
{
  namespace xcurl
  {
    inline constexpr std::uint32_t max_transfers (64);
    inline constexpr std::uint32_t max_multis (8);

    transfer*
    open_transfer () noexcept;

    void
    close_transfer (transfer&) noexcept;

    transfer*
    as_transfer (CURL*) noexcept;

    transfer*
    transfer_of (CURL* easy) noexcept;

    multi*
    open_multi () noexcept;

    void
    close_multi (multi&) noexcept;

    multi*
    as_multi (CURLM*) noexcept;
  }
}
