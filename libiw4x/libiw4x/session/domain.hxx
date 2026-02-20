#pragma once

namespace iw4x
{
  namespace session
  {
    // IW4x identifier (32-byte digest).
    //
    struct build_compatibility_tag
    {
      array<uint8_t, 32> digest;

      bool operator== (const build_compatibility_tag&) const = default;
    };

    // Session roles.
    //
    struct host_role {};

    // Metadata advertised to potential players.
    //
    struct session_metadata
    {
      build_compatibility_tag build;
    };
  }
}
