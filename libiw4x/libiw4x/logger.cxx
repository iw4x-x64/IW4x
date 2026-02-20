#include <libiw4x/logger.hxx>

namespace iw4x
{
  logger* active_logger (nullptr);

  logger::
  logger ()
  {
    // Start Quill backend thread.
    //
    // Note that Quill backend is kept responsive (no sleep) and is allowed to
    // exit without draining queues to avoid MinGW-specific shutdown hangs.
    //
    quill::Backend::start ({
      .enable_yield_when_idle               = true,
      .sleep_duration                       = 0ns,
      .wait_for_queues_to_empty_before_exit = false,
      .check_printable_char                 = {},
      .log_level_short_codes                =
      {
        "3", "2", "1", "D", "I", "N", "W", "E", "C", "B", "_"
      }
    });

    // Console sink.
    //
    // Writes to stdout with ANSI colour codes where the terminal supports them.
    // Quill's ConsoleSink applies colour by severity level.
    //
    auto cs (quill::Frontend::create_or_get_sink<quill::ConsoleSink> ("cs"));

    // Rotating file sink.
    //
    // Rotates hourly and retains the last four files so that a single session
    // never fills the disk. Oldest files are overwritten once the backup limit
    // is reached.
    //
    auto fs (quill::Frontend::create_or_get_sink<quill::RotatingFileSink> (
      "iw4x.log", []
    {
      quill::RotatingFileSinkConfig c;
      c.set_rotation_frequency_and_interval ('H', 1);
      c.set_max_backup_files (4);
      c.set_overwrite_rolled_files (true);
      c.set_filename_append_option (quill::FilenameAppendOption::StartDateTime);
      return c;
    } ()));

    // Configure format pattern applied to all category loggers.
    //
    quill::PatternFormatterOptions fmt (
      "%(time) [%(log_level_short_code)] %(logger:<16) %(caller_function) "
      "%(short_source_location:<32) %(message)",
      "%H:%M:%S.%Qms",
      quill::Timezone::LocalTime);
  }
}
