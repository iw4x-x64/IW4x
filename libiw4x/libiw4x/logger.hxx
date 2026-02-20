#pragma once

#include <libiw4x/export.hxx>

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/core/PatternFormatterOptions.h>
#include <quill/sinks/ConsoleSink.h>
#include <quill/sinks/RotatingFileSink.h>

namespace iw4x
{
  class LIBIW4X_SYMEXPORT logger
  {
  public:
    logger ();
    ~logger ();

    logger (const logger&) = delete;
    logger& operator= (const logger&) = delete;

    logger (logger&&) = delete;
    logger& operator= (logger&&) = delete;
  };

  // Process-lifetime logger instance.
  //
  LIBIW4X_SYMEXPORT extern logger* active_logger;
}
