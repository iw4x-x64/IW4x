#include <libiw4x/logger.hxx>

#include <string_view>

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/LogFunctions.h>
#include <quill/Logger.h>
#include <quill/sinks/ConsoleSink.h>

#include <quill/bundled/fmt/base.h>
#include <quill/bundled/fmt/format.h>

namespace iw4x
{
  namespace logger
  {
    namespace detail
    {
      static_assert (static_cast<quill::LogLevel> (level::trace3) ==
                     quill::LogLevel::TraceL3);
      static_assert (static_cast<quill::LogLevel> (level::trace2) ==
                     quill::LogLevel::TraceL2);
      static_assert (static_cast<quill::LogLevel> (level::trace1) ==
                     quill::LogLevel::TraceL1);
      static_assert (static_cast<quill::LogLevel> (level::info) ==
                     quill::LogLevel::Info);
      static_assert (static_cast<quill::LogLevel> (level::warning) ==
                     quill::LogLevel::Warning);
      static_assert (static_cast<quill::LogLevel> (level::error) ==
                     quill::LogLevel::Error);

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

          // Normally Quill can decide whether to emit ANSI colour
          // sequences by inspecting the terminal. This doesn't work
          // reliably for us in some of the environments in which IW4x
          // runs. One such case is Wine attached to a terminal that does
          // understand escape sequences, where the terminal can still end
          // up being detected as not supporting them.
          //
          // Since our console does support these sequences in the
          // environments where we use it, force colour here instead of
          // depending on that detection.
          //
          cs.set_colour_mode (quill::ConsoleSinkConfig::ColourMode::Always);

          auto s (
            quill::Frontend::create_or_get_sink<quill::ConsoleSink> ("console",
                                                                     cs));

          quill::Logger* l (
            quill::Frontend::create_or_get_logger ("iw4x", std::move (s), f));

          return l->set_log_level (quill::LogLevel::TraceL3), l;
        }

        quill::Logger* logger (init ());

        using context = fmtquill::format_context;

        using buffer = fmtquill::basic_memory_buffer<char, 256>;

        constexpr std::string_view null_string ("(null)");

        std::string_view
        text (const argument& a) noexcept
        {
          if (a.string == nullptr)
            return null_string;

          return std::string_view (a.string,
                                   a.size == argument::unmeasured
                                     ? __builtin_strlen (a.string)
                                     : a.size);
        }
      }

      void
      log (level l,
           const location& where,
           const char* format,
           const argument* args,
           std::size_t n) noexcept
      {
        quill::LogLevel ql (static_cast<quill::LogLevel> (l));

        if (logger == nullptr || !logger->should_log_statement (ql))
          return;

        if (n > argument::max_count)
          n = argument::max_count;

        buffer b;

        try
        {
          fmtquill::basic_format_arg<context> fa[argument::max_count];

          for (std::size_t i (0); i != n; ++i)
          {
            const argument& a (args[i]);

            switch (a.which)
            {
            case argument::kind::signed_integer:
              fa[i] = a.signed_integer;
              break;
            case argument::kind::unsigned_integer:
              fa[i] = a.unsigned_integer;
              break;
            case argument::kind::real:
              fa[i] = a.real;
              break;
            case argument::kind::boolean:
              fa[i] = a.boolean;
              break;
            case argument::kind::character:
              fa[i] = a.character;
              break;
            case argument::kind::string:
              fa[i] = text (a);
              break;
            case argument::kind::pointer:
              fa[i] = a.pointer;
              break;
            }
          }

          fmtquill::vformat_to (
            fmtquill::appender (b),
            format,
            fmtquill::basic_format_args<context> (fa,
                                                  static_cast<int> (n)));
        }
        catch (...)
        {
          b.clear ();
          b.append (format, format + __builtin_strlen (format));
        }

        // There is no tag associated with these messages for now, hence
        // the empty string. The level already carries the distinction we
        // care about here and putting fail/info/l1 and so on into the
        // message would only duplicate it.
        //
        quill::log (
          logger,
          "",
          ql,
          "{}",
          quill::SourceLocation (where.file, where.function, where.line),
          std::string_view (b.data (), b.size ()));
      }
    }

    void
    stop () noexcept
    {
      quill::Backend::stop ();
    }
  }
}
