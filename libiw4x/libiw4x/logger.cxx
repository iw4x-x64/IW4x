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
      quill::BackendOptions o;
      o.log_level_short_codes = {
        "3", "2", "1", "D", "I", "N", "W", "E", "C", "B", "_"
      };

      quill::Backend::start (o);
      quill::ConsoleSinkConfig cs;
      quill::PatternFormatterOptions f (
        "[%(log_level_short_code)] "
        "%(short_source_location:<24) "
        "%(message)");

      // Normally Quill can decide whether to emit ANSI colour sequences
      // by inspecting the terminal. This doesn't work reliably for us
      // in some of the environments in which IW4x runs. One such case
      // is Wine attached to a terminal that does understand escape
      // sequences, where the terminal can still end up being detected
      // as not supporting them.
      //
      // Since our console does support these sequences in the
      // environments where we use it, force colour here instead of
      // depending on that detection.
      //
      cs.set_colour_mode (quill::ConsoleSinkConfig::ColourMode::Always);

      auto s (
        quill::Frontend::create_or_get_sink <quill::ConsoleSink> ("console",
                                                                  cs));

      quill::Logger* l (
        quill::Frontend::create_or_get_logger ("iw4x", move (s), f));

      return l->set_log_level (quill::LogLevel::TraceL3), l;
    }
  }

  quill::Logger* logger (init ());
}
