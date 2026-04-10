#pragma once

namespace iw4x
{
  namespace demonware
  {
    [[gnu::ms_abi]] void
    bd_log_message (int type,
                    const char* base_channel,
                    const char* channel,
                    const char* file,
                    const char* function,
                    int line,
                    const char* fmt,
                    ...);
  }
}
