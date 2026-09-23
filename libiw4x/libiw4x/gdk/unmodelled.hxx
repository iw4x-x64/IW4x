#pragma once

#include <cstddef>

#include <libiw4x/gdk/types.hxx>
#include <libiw4x/gdk/vtable.hxx>

namespace iw4x
{
  namespace gdk
  {
    struct xpackage
    {
      static constexpr char label[] = "XPackage";

      static constexpr guid api
      {
        0xAF406016, 0xE850, 0x4AA8,
        {0xA8, 0x8D, 0x2F, 0x3D, 0xCB, 0x9D, 0xAC, 0x7E}
      };

      static constexpr guid id
      {
        0xE2A4734B, 0x2F4A, 0x456D,
        {0xAA, 0x8F, 0xD0, 0x65, 0xE0, 0x4F, 0xB2, 0x09}
      };

      static constexpr std::size_t declined[]
      {
        0x58,
        0xB0,
        0xC8,
        0xD0,
        0x128,
        0x130,
        0x138,
        0x140
      };
    };

    struct xgame_event
    {
      static constexpr char label[] = "XGameEvent";

      static constexpr guid api
      {
        0xBBFBDCC7, 0xBFE7, 0x409B,
        {0xA5, 0xCA, 0xED, 0xF0, 0x54, 0x96, 0x0B, 0x4D}
      };

      static constexpr guid id {api};

      static constexpr std::size_t declined[] {0x18};
    };

    struct xunidentified
    {
      static constexpr char label[] = "unidentified 2549F142";

      static constexpr guid api
      {
        0x973A344E, 0x24BF, 0x4D0F,
        {0x84, 0x57, 0x56, 0xC5, 0x34, 0x89, 0x2B, 0x29}
      };

      static constexpr guid id
      {
        0x2549F142, 0x6419, 0x4A06,
        {0x97, 0xB5, 0x93, 0x1A, 0xAB, 0x7C, 0x2F, 0x34}
      };

      static constexpr std::size_t declined[] {0x18};
    };
  }
}
