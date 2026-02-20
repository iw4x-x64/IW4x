#include <libiw4x/logger.hxx>

using namespace quill;

namespace iw4x
{
  logger* active_logger (nullptr);

  namespace
  {
    // Construct a per-category Quill logger sharing the supplied sinks.
    //
    // The logger is registered in Quill's global manager under name and
    // appears verbatim in the %(logger) pattern field of every line emitted
    // by that category.
    //
    Logger*
    make_category_logger (string const&                      name,
                          initializer_list<shared_ptr<Sink>> sinks,
                          PatternFormatterOptions const&     fmt,
                          LogLevel                           threshold)
    {
      Logger* l (Frontend::create_or_get_logger (name, sinks, fmt));
      l->set_log_level (threshold);
      return l;
    }
  }

  logger::
  logger ()
  {
    // Start Quill backend thread.
    //
    // Note that Quill backend is kept responsive (no sleep) and is allowed to
    // exit without draining queues to avoid MinGW-specific shutdown hangs.
    //
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

    // Console sink.
    //
    // Writes to stdout with ANSI colour codes where the terminal supports
    // them. Quill's ConsoleSink applies colour by severity level.
    //
    auto cs (Frontend::create_or_get_sink<ConsoleSink> ("cs"));

    // Rotating file sink.
    //
    // Rotates hourly and retains the last four files so that a single session
    // never fills the disk. Oldest files are overwritten once the backup limit
    // is reached.
    //
    RotatingFileSinkConfig r;
    r.set_rotation_frequency_and_interval ('H', 1);
    r.set_max_backup_files (4);
    r.set_overwrite_rolled_files (true);
    r.set_filename_append_option (quill::FilenameAppendOption::StartDateTime);

    auto fs (Frontend::create_or_get_sink<RotatingFileSink> (
      "iw4x.log", r));

    // Configure format pattern applied to all category loggers.
    //
    PatternFormatterOptions fmt (
      "%(time) [%(log_level_short_code)] %(logger:<16) %(caller_function) "
      "%(short_source_location:<32) %(message)",
      "%H:%M:%S.%Qms",
      Timezone::LocalTime);

    // One dedicated Quill logger per category.
    //
    // All loggers share the same two sinks and the same format pattern. In
    // other words, they differ only in their registered name and their default
    // threshold.
    //
    using namespace categories;

    log::detail::category_logger_ref<iw4x> () =
      make_category_logger (
        string (log::category_policy<iw4x>::name),
        {cs, fs}, fmt,
        log::category_policy<iw4x>::threshold);

    log::detail::category_logger_ref<steam> () =
      make_category_logger (
        string (log::category_policy<steam>::name),
        {cs, fs}, fmt,
        log::category_policy<steam>::threshold);

    // Development builds open the full trace range on every category so that
    // internals are visible without adjusting per-category thresholds manually.
    // Note that the compile-time compiled_minimum_level already admits trace
    // statements in LIBIW4X_DEVELOP builds, so this runtime adjustment is what
    // actually allows them to reach the sinks.
    //
#if LIBIW4X_DEVELOP
    log::detail::category_logger_ref<iw4x> ()->set_log_level (LogLevel::TraceL3);
    log::detail::category_logger_ref<steam> ()->set_log_level (LogLevel::TraceL3);
#endif
  }

  logger::~logger ()
  {
    // Null all category logger pointers before stopping the backend so that any
    // log call racing with this destructor safely drops the message rather than
    // writing to a logger whose backend is no longer running.
    //
    // Note that the write here is not atomic but the destructor is only called
    // from the main thread during an orderly shutdown, so no concurrent log
    // calls should be in flight when it runs.

    using namespace categories;

    log::detail::category_logger_ref<iw4x> () = nullptr;
    log::detail::category_logger_ref<steam> () = nullptr;

    Backend::stop ();
  }

  void
  logger::flush () noexcept
  {
    // All category loggers feed the same backend thread via per-thread SPSC
    // queues. Sending a flush sentinel through any one category logger
    // guarantees that the backend has processed all messages enqueued before
    // the sentinel, regardless of which category produced them. iw4x is used as
    // the representative here because it is always initialised.
    //
    Logger* l (log::detail::category_logger_ref<categories::iw4x> ());

    if (l)
      l->flush_log ();
  }
}
