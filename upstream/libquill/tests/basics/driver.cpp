#include <string>

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/Logger.h>
#include <quill/LogMacros.h>
#include <quill/sinks/ConsoleSink.h>

#undef NDEBUG
#include <cassert>

static_assert (quill::Version == 120100);

int
main ()
{
  quill::BackendOptions opts;
  quill::Backend::start (opts);

  auto sink (
    quill::Frontend::create_or_get_sink<quill::ConsoleSink> ("sink_id_1"));

  quill::Logger* log (
    quill::Frontend::create_or_get_logger (
      "root",
      std::move (sink),
      quill::PatternFormatterOptions {
        "%(time) [%(process_id)] [%(thread_id)] %(logger) - %(message)",
        "%Y-%m-%d %H:%M:%S.%Qms",
        quill::Timezone::GmtTime,
        false},
      quill::ClockSourceType::System));

  log->set_log_level (quill::LogLevel::TraceL3);

  LOG_TRACE_L3 (log, "This is a log trace l3 example {}", 1);
  LOG_TRACE_L2 (log, "This is a log trace l2 example {} {}", 2, 2.3);
  LOG_TRACE_L1 (log, "This is a log trace l1 {} example", "string");
  LOG_DEBUG (log, "This is a log debug example {}", 4);
  LOG_INFO (log, "This is a log info example {}", sizeof (std::string));
  LOG_WARNING (log, "This is a log warning example {}", sizeof (std::string));
  LOG_ERROR (log, "This is a log error example {}", sizeof (std::string));
  LOG_CRITICAL (log, "This is a log critical example {}", sizeof (std::string));

  return 0;
}
