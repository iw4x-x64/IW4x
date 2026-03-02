#include <libiw4x/logger.hxx>

#include <string>
#include <initializer_list>

#include <quill/Frontend.h>
#include <quill/sinks/ConsoleSink.h>
#include <quill/sinks/RotatingFileSink.h>

using namespace std;
using namespace quill;

namespace iw4x
{
  class logger* logger (nullptr);

  namespace
  {
    Logger*
    register_category (string const&                      n,
                       initializer_list<shared_ptr<Sink>> s,
                       PatternFormatterOptions const&     f,
                       LogLevel                           t)
    {
      Logger* l (Frontend::create_or_get_logger (n, s, f));
      l->set_log_level (t);
      return l;
    }
  }

  logger::
  logger ()
  {
    Backend::start ({
      .enable_yield_when_idle               = true,
      .sleep_duration                       = 0ns,
      .wait_for_queues_to_empty_before_exit = false,
      .check_printable_char                 = {},
      .log_level_short_codes                =
      {
        "3", "2", "1", "D", "I", "N", "W", "E", "C", "B", "_"
      }
    });

    ConsoleSinkConfig c;
    c.set_colour_mode (ConsoleSinkConfig::ColourMode::Never);

    RotatingFileSinkConfig r;
    r.set_rotation_frequency_and_interval ('H', 1);
    r.set_max_backup_files (24);
    r.set_rotation_max_file_size (1'000'000'000);
    r.set_overwrite_rolled_files (true);
    r.set_filename_append_option (FilenameAppendOption::StartDateTime);

    auto cs (Frontend::create_or_get_sink<ConsoleSink>      ("cs",       c));
    auto fs (Frontend::create_or_get_sink<RotatingFileSink> ("iw4x.log", r));

    PatternFormatterOptions pf (
      "%(time) [%(log_level_short_code)] %(logger:<16) %(caller_function:<32) "
      "%(short_source_location:<24) %(message)",
      "%H:%M:%S.%Qms",
      Timezone::LocalTime);

    using namespace log::categories;

    log::detail::logger<iw4x> () =
      register_category (string (log::policy<iw4x>::name),
                         {cs, fs},
                         pf,
                         log::policy<iw4x>::threshold);

    // In development builds, we blow the doors wide open and allow all trace
    // statements through so internals are visible. The compile-time minimum
    // level already permits this in LIBIW4X_DEVELOP mode, so we just need to
    // drop the runtime threshold here so they actually hit the sinks.
    //
#if LIBIW4X_DEVELOP
    log::detail::logger<iw4x> ()->set_log_level (LogLevel::TraceL3);
#endif
  }

  logger::
  ~logger ()
  {
    using namespace log::categories;

    log::detail::logger<iw4x> () = nullptr;

    Backend::stop ();
  }
}
