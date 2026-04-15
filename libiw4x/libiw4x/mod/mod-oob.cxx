#include <libiw4x/mod/mod-oob.hxx>

#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <libiw4x/logger.hxx>
#include <libiw4x/scheduler.hxx>

using namespace std;

namespace iw4x
{
  namespace mod
  {
    namespace
    {
      oob_module* oob_c (nullptr);
    }

    extern "C"
    {
      bool
      oob_dispatch (const network_address* a, const message* m)
      {
        try
        {
          return oob_c->dispatch (*a, *m);
        }
        catch (const exception& e)
        {
          log::error << e.what ();
          exit (1);
        }
      }

      void
      oob_dispatch_stub ();
    }

    bool oob_module::
    dispatch (const network_address& a, const message& m)
    {
      // We need at least the OOB header (4 bytes of 0xFF) and some payload. If
      // the packet is too small or missing data, there is nothing for us to do.
      //
      if (m.data == nullptr || m.current_size <= 4)
        return false;

      vector<string> args;

      const char* p (m.data + 4);
      const char* end (m.data + m.current_size);

      string token;
      bool in_quotes (false);

      for (; p < end; ++p)
      {
        char c (*p);

        // Bail out if we hit a null terminator or a newline. This usually
        // means the engine has finished writing the command line sequence.
        //
        if (c == '\0' || c == '\n')
          break;

        // If we encounter a quote, toggle in_quotes to not mistakenly treat
        // them as token delimiters.
        //
        if (c == '"')
        {
          in_quotes = !in_quotes;
          continue;
        }

        // When outside of quotes, any whitespace character acts as a
        // boundary between arguments. If we have accumulated a valid token,
        // push it back and clear it for the next one.
        //
        if (!in_quotes && (c == ' ' || c == '\t' || c == '\r'))
        {
          if (!token.empty ())
          {
            args.push_back (move (token));
            token.clear ();
          }
          continue;
        }

        // Otherwise, simply accumulate the character into the current token.
        //
        token += c;
      }

      // We might reach the end of the buffer without a trailing space. If so,
      // make sure we flush the last collected token into our arguments list.
      //
      if (!token.empty ())
        args.push_back (move (token));

      // At this point we must have at least one token (the command itself)
      // to proceed.
      //
      if (args.empty ())
        return false;

      // The first argument is always the command name. We look it up in our
      // registered handlers map. If we don't know about it, we trace it out
      // and return false so the underlying engine can handle it.
      //
      const string& n (args[0]);
      auto i (handlers_.find (n));

      if (i == handlers_.end ())
      {
        log::trace_l2 << "deferring oob command '" << n << "' to engine";
        return false;
      }

      log::trace_l1 << "dispatching oob command '" << n << "'";

      // Invoke the registered handler.
      //
      try
      {
        i->second (a, args);
        return true;
      }
      catch (const exception& e)
      {
        log::error << "unable to dispatch '" << n << "': " << e.what ();
        exit (1);
      }
    }

    void oob_module::
    register_handler (const string& n, handler h)
    {
      if (n.empty ())
        throw invalid_argument ("empty name");

      log::debug << "registered oob handler '" << n << "'";
      handlers_.emplace (n, move (h));
    }

    oob_module::
    oob_module ()
    {
      oob_c = this;

      // Install the OOB packet interception loop.
      //
      // Note that we bypass our standard detour utility for this hook. The
      // target site (CL_DispatchConnectionlessPacket) doesn't have a
      // conventional function prologue, and the dispatch path needs complete
      // control over the call frame.
      //
      {
        uintptr_t t (0x1400F6040);
        uintptr_t h (reinterpret_cast<uintptr_t> (&oob_dispatch_stub));

        array<unsigned char, 14> sequence
        {{
          static_cast<unsigned char> (0xFF),
          static_cast<unsigned char> (0x25),
          static_cast<unsigned char> (0x00),
          static_cast<unsigned char> (0x00),
          static_cast<unsigned char> (0x00),
          static_cast<unsigned char> (0x00),
          static_cast<unsigned char> (h       & 0xFF),
          static_cast<unsigned char> (h >>  8 & 0xFF),
          static_cast<unsigned char> (h >> 16 & 0xFF),
          static_cast<unsigned char> (h >> 24 & 0xFF),
          static_cast<unsigned char> (h >> 32 & 0xFF),
          static_cast<unsigned char> (h >> 40 & 0xFF),
          static_cast<unsigned char> (h >> 48 & 0xFF),
          static_cast<unsigned char> (h >> 56 & 0xFF)
        }};

        memmove (reinterpret_cast<void*> (t),
                 sequence.data (), sequence.size ());
      }
    }
  }
}
