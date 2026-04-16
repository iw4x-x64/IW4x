#pragma once

namespace iw4x
{
  namespace demonware
  {
    void
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
