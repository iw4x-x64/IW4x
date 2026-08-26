#include <libiw4x/logger.hxx>

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/sinks/ConsoleSink.h>
#include <quill/sinks/FileSink.h>

namespace iw4x::logger::detail
{
  namespace
  {
    quill::Logger*
    init ()
    {
      quill::Backend::start ();

      auto cs (
        quill::Frontend::create_or_get_sink <quill::ConsoleSink> ("console"));

      return quill::Frontend::create_or_get_logger ("iw4x", move (cs));
    }
  }

  quill::Logger* logger (init ());
}
