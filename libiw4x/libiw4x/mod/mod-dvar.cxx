#include <libiw4x/mod/mod-dvar.hxx>

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <cinttypes>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include <libiw4x/detour.hxx>
#include <libiw4x/import.hxx>
#include <libiw4x/logger.hxx>

using namespace std;

namespace iw4x
{
  namespace mod
  {
    namespace
    {
      // TLS slot index is stored at 0x148CA3A28 as an integer.
      //
      // The TLS block layout (byte offsets from block base) is organized as
      // follows:
      //
      //   +0x20 modified_flags (int) - accumulates flags of modified dvars.
      //   +0x24 allowed_flags  (int) - flags permitted in the current context.
      //
      // Keep in mind that during Dvar_Init, we set allowed_flags to -1 (meaning
      // all flags are allowed). We do this so that early dvar registrations are
      // not incorrectly restricted before the context is fully established.
      //
      constexpr uintptr_t tls_index_address   (0x148CA3A28);
      constexpr ptrdiff_t tls_modified_offset (0x20);
      constexpr ptrdiff_t tls_allowed_offset  (0x24);

      // Address of a byte flag that the engine sets to 1 when Dvar_Init has
      // run.
      //
      constexpr uintptr_t dvar_initialized_flag (0x14673D271);

      // Address of a byte flag set to 1 when autoexec processing is active.
      //
      // When we notice this flag is set, set_command() needs to mark dvars with
      // DVAR_AUTOEXEC and update their reset values so that they act as
      // defaults.
      //
      constexpr uintptr_t dvar_autoexec_flag (0x14673D272);

      // Address of a byte flag indicating whether cheats are enabled.
      //
      // We rely on this inside set_variant() to enforce the DVAR_CHEAT guard,
      // so that players don't bypass cheat protections.
      //
      constexpr uintptr_t cheats_enabled_flag (0x14046CFB0);

      // Function that asserts the current call is happening on the main
      // thread, and that the dvar system is in a safe state for cross-thread
      // notifications.
      //
      // Note that we must call this before notifying sv_dvar_modifiedFlags.
      //
      constexpr uintptr_t sv_notify_fn (0x140240EB0);

      // Function called at the start of set_variant.
      //
      constexpr uintptr_t pre_set_variant_fn (0x1401FB230);

      // Retrieve the per-thread TLS block used for dvar tracking.
      //
      // We can call this early on as it returns a null pointer if TLS has not
      // been set up yet.
      //
      void*
      engine_tls ()
      {
        void** slots (nullptr);

        // TODO(wroyca): Will not compile with MSVC as microsoft pulled
        // the plug on inline asm support for x64.
#if defined(__x86_64__)
        asm volatile ("movq %%gs:0x58, %0"
                      : "=r"(slots));
#endif

        if (slots == nullptr)
          return nullptr;

        int index (*reinterpret_cast<int*> (tls_index_address));
        return slots[index];
      }

      // Mark the provided flags in the TLS registration tracking slot.
      //
      // We call this right before any dvar registration to keep a record of
      // which flag categories were used. If TLS is not ready yet, we skip it,
      // which is expected during early initialization.
      //
      void
      mark_registration_flags (DvarFlags flags)
      {
        void* tls (engine_tls ());

        if (tls == nullptr)
          return;

        int& modified (*reinterpret_cast<int*> (
          static_cast<unsigned char*> (tls) + tls_modified_offset));
        modified |= static_cast<int> (flags);
      }

      // Mark a dvar's flags as modified in the TLS modified_flags slot, subject
      // to the allowed_flags mask.
      //
      // For userinfo or systeminfo dvars that are updated on the main thread,
      // we also need to notify the server subsystem via sv_notify_fn so that
      // changes propagate.
      //
      void
      mark_modified_flags (dvar_t* dvar)
      {
        if (dvar == nullptr)
          return;

        void* tls (engine_tls ());

        if (tls == nullptr)
          return;

        int flags (static_cast<int> (dvar->flags));
        int& modified (*reinterpret_cast<int*> (
          static_cast<unsigned char*> (tls) + tls_modified_offset));
        int& allowed (*reinterpret_cast<int*> (
          static_cast<unsigned char*> (tls) + tls_allowed_offset));

        if ((flags & ~allowed) == 0)
        {
          modified |= flags;
          return;
        }

        // The dvar happens to have flags outside the current allowed set. If we
        // are on the main thread, we must notify the server subsystem. On other
        // threads, we can safely skip this.
        //
        if (!Sys_IsMainThread ())
          return;

        if ((flags & (DVAR_USERINFO | DVAR_SYSTEMINFO)) != 0)
          return;

        reinterpret_cast<void (*) ()> (sv_notify_fn) ();

        if (*sv_dvar_modifiedFlags != nullptr)
          **sv_dvar_modifiedFlags |= flags;
      }

      // Locking protocol implementation.
      //
      // Dvar relies on a custom reader-writer spinlock (internally called
      // fast_critical_section or g_dvarCritSect) to guard its hash table
      // accesses. We must replicate this exact protocol here to prevent
      // deadlocks: multiple concurrent readers are fine, but writers have to
      // exclude all other threads.
      //
      struct read_lock
      {
        read_lock ()
        {
          InterlockedIncrement (&g_dvarCritSect->readCount);

          while (g_dvarCritSect->writeCount != 0)
            Sleep (1);
        }

        ~read_lock ()
        {
          InterlockedDecrement (&g_dvarCritSect->readCount);
        }
      };

      struct write_lock
      {
        HANDLE thread {nullptr};
        int old_priority {0};

        write_lock ()
        {
          thread = GetCurrentThread ();
          old_priority = GetThreadPriority (thread);

          // We elevate the thread to normal priority during the critical
          // section. This is a deliberate design choice to reduce
          // priority-inversion stalls that we are otherwise prone to.
          //
          if (old_priority < 0)
            SetThreadPriority (thread, 0);

          for (;;)
          {
            if (g_dvarCritSect->writeCount == 0)
            {
              long count (
                InterlockedExchangeAdd (&g_dvarCritSect->writeCount, 1) + 1);

              if (count == 1 && g_dvarCritSect->readCount == 0)
                break;

              InterlockedDecrement (&g_dvarCritSect->writeCount);
            }

            Sleep (1);
          }
        }

        ~write_lock ()
        {
          InterlockedDecrement (&g_dvarCritSect->writeCount);

          if (old_priority < 0)
            SetThreadPriority (thread, old_priority);
        }
      };
    }

    namespace
    {
      bool
      readable_page (const void* address)
      {
        MEMORY_BASIC_INFORMATION info {};

        if (address == nullptr ||
            VirtualQuery (address, &info, sizeof (info)) == 0 ||
            info.State != MEM_COMMIT || (info.Protect & PAGE_GUARD) != 0)
          return false;

        switch (info.Protect & 0xFF)
        {
          case PAGE_READONLY:
          case PAGE_READWRITE:
          case PAGE_WRITECOPY:
          case PAGE_EXECUTE_READ:
          case PAGE_EXECUTE_READWRITE:
          case PAGE_EXECUTE_WRITECOPY:
            return true;

          default:
            return false;
        }
      }

      bool
      executable_page (const void* address)
      {
        MEMORY_BASIC_INFORMATION info {};

        if (address == nullptr ||
            VirtualQuery (address, &info, sizeof (info)) == 0 ||
            info.State != MEM_COMMIT || (info.Protect & PAGE_GUARD) != 0)
          return false;

        switch (info.Protect & 0xFF)
        {
          case PAGE_EXECUTE:
          case PAGE_EXECUTE_READ:
          case PAGE_EXECUTE_READWRITE:
          case PAGE_EXECUTE_WRITECOPY:
            return true;

          default:
            return false;
        }
      }

      bool
      readable_c_string (const char* text)
      {
        if (text == nullptr)
          return true;

        constexpr size_t max_description_length (8192);
        size_t offset (0);

        while (offset < max_description_length)
        {
          const char* address (text + offset);
          MEMORY_BASIC_INFORMATION info {};

          if (!readable_page (address) ||
              VirtualQuery (address, &info, sizeof (info)) == 0)
            return false;

          uintptr_t base (reinterpret_cast<uintptr_t> (info.BaseAddress));
          uintptr_t end (base + info.RegionSize);
          uintptr_t current (reinterpret_cast<uintptr_t> (address));

          size_t remaining (end > current ? static_cast<size_t> (end - current)
                                          : 0);

          if (remaining == 0)
            return false;

          size_t chunk (min (remaining, max_description_length - offset));

          for (size_t i (0); i < chunk; ++i)
          {
            if (address[i] == '\0')
              return true;
          }

          offset += chunk;
        }

        return false;
      }

      const char*
      log_safe (const char* text)
      {
        if (text == nullptr)
          return "<null>";

        if (!readable_c_string (text))
          return "<unreadable>";

        return text;
      }

      // Pool membership validation.
      //
      // Here we validate that a given dvar_t pointer refers to an entry in the
      // static dvarPool array, computing its index along the way.
      //
      // Note that we rely on pool membership to index into our per-dvar
      // description array (defined just below).
      //
      bool
      pool_index (const dvar_t* dvar, size_t& index)
      {
        if (dvar == nullptr)
          return false;

        uintptr_t base (reinterpret_cast<uintptr_t> (dvarPool));
        uintptr_t address (reinterpret_cast<uintptr_t> (dvar));
        uintptr_t span (sizeof (dvar_t) * dvar_pool_capacity);

        if (address < base || address >= base + span)
          return false;

        uintptr_t offset (address - base);

        if ((offset % sizeof (dvar_t)) != 0)
          return false;

        index = offset / sizeof (dvar_t);
        return index < dvar_pool_capacity;
      }

      // Per-dvar description storage system.
      //
      // The description field is tied to the SL system, which is not quite
      // reliable. In other words, we replace it with a parallel array of
      // heap-allocated C strings, indexed by pool position. This acts as the
      // source for dvar descriptions returned to callers.
      //
      // Keep in mind that each non-null entry is a raw heap allocation created
      // with `new char[]`. It will be manually freed and replaced if
      // set_description() gets called again for the exact same slot.
      //
      const char* dvar_descriptions[dvar_pool_capacity] {};
      bool dvar_tombstones[dvar_pool_capacity] {};

      bool
      tombstoned (const dvar_t* dvar)
      {
        size_t index (0);
        return pool_index (dvar, index) && dvar_tombstones[index];
      }

      // Store a description for the specified dvar.
      //
      // If the dvar isn't in the pool, we discard the update. We also
      // fall back to an empty string if the description is
      // unreadable.
      //
      void
      set_description (dvar_t* dvar, const char* description)
      {
        size_t index (0);

        if (!pool_index (dvar, index))
          return;

        if (!readable_c_string (description))
        {
          log::warning << "dvar: ignoring unreadable description for '"
                       << log_safe (dvar->name) << "' description="
                       << static_cast<const void*> (description);
          description = "";
        }

        const char* text (description != nullptr ? description : "");

        // Try to avoid an allocation if the text is identical.
        //
        const char* old (dvar_descriptions[index]);

        if (old != nullptr && strcmp (old, text) == 0)
          return;

        size_t len (strlen (text));
        char* copy (new char[len + 1]);
        memcpy (copy, text, len + 1);

        dvar_descriptions[index] = copy;

        if (old != nullptr)
          delete[] old;
      }

      // Retrieve the stored description, returning an empty string if
      // none is available.
      //
      const char*
      get_description (const dvar_t* dvar)
      {
        size_t index (0);

        if (!pool_index (dvar, index))
          return "";

        return dvar_descriptions[index] != nullptr ? dvar_descriptions[index]
                                                   : "";
      }

      // Memory-safe value copy.
      //
      // Notice that vector types (like FLOAT_2/3/4 and FLOAT_3_COLOR) only
      // occupy the leading component bytes of the DvarValue union. We copy only
      // the valid bytes as a full struct copy would smear garbage into unused
      // union members, which breaks the values_equal() comparison down the
      // line.
      //
      void
      copy_value (DvarValue& dst, const DvarValue& src, dvarType type)
      {
        memset (&dst, 0, sizeof (dst));

        switch (type)
        {
          case DVAR_TYPE_FLOAT_2:
            memcpy (&dst, &src, sizeof (float) * 2);
            break;

          case DVAR_TYPE_FLOAT_3:
          case DVAR_TYPE_FLOAT_3_COLOR:
            memcpy (&dst, &src, sizeof (float) * 3);
            break;

          case DVAR_TYPE_FLOAT_4:
            memcpy (&dst, &src, sizeof (float) * 4);
            break;

          default:
            memcpy (&dst, &src, sizeof (dst));
            break;
        }
      }

      // Canonical string value helpers.
      //
      // String dvars maintain up to three live string pointers: current,
      // latched, and reset. To keep memory allocations low and fast, we reuse
      // the existing pointers whenever the supplied value is either
      // pointer-equal or content-equal to one of the slots we already have.
      //
      DvarValue
      canonical_string_value (const dvar_t* dvar, const char* value)
      {
        DvarValue result {};

        if (value != nullptr && value == dvar->current.string)
        {
          result.string = dvar->current.string;
          return result;
        }

        if (value != nullptr && value == dvar->reset.string)
        {
          result.string = dvar->reset.string;
          return result;
        }

        result.string = CopyStringInternal (value != nullptr ? value : "");
        return result;
      }

      DvarValue
      canonical_current_string_value (const dvar_t* dvar, const char* value)
      {
        DvarValue result {};

        if (value != nullptr && value == dvar->latched.string)
        {
          result.string = dvar->latched.string;
          return result;
        }

        if (value != nullptr && value == dvar->reset.string)
        {
          result.string = dvar->reset.string;
          return result;
        }

        result.string = CopyStringInternal (value != nullptr ? value : "");
        return result;
      }

      DvarValue
      canonical_reset_string_value (const dvar_t* dvar, const char* value)
      {
        DvarValue result {};

        if (value != nullptr && value == dvar->current.string)
        {
          result.string = dvar->current.string;
          return result;
        }

        if (value != nullptr && value == dvar->latched.string)
        {
          result.string = dvar->latched.string;
          return result;
        }

        result.string = CopyStringInternal (value != nullptr ? value : "");
        return result;
      }

      int
      string_to_enum (const DvarLimits& domain, const char* string)
      {
        if (string == nullptr)
          string = "";

        // It is safer to try an exact case-insensitive match first.
        //
        for (int i (0); i < domain.enumeration.stringCount; ++i)
        {
          if (I_stricmp (string, domain.enumeration.strings[i]) == 0)
            return i;
        }

        // If the input turns out to be a pure digit sequence, we must treat it
        // as an index.
        //
        const unsigned char* p (
          reinterpret_cast<const unsigned char*> (string));

        int value (0);

        for (; *p != '\0'; ++p)
        {
          if (*p < '0' || *p > '9')
          {
            // At this point, the input is not numeric, so we fall back to
            // attempting a prefix match.
            //
            size_t length (strlen (string));

            for (int i (0); i < domain.enumeration.stringCount; ++i)
            {
              if (I_strnicmp (string,
                              domain.enumeration.strings[i],
                              static_cast<int> (length)) == 0)
                return i;
            }

            return -1337; // Act as a sentinel for failure.
          }

          value = value * 10 + (*p - '0');
        }

        return (value >= 0 && value < domain.enumeration.stringCount) ? value
                                                                      : -1337;
      }

      unsigned char
      color_byte (float v)
      {
        v = max (0.0f, min (1.0f, v));
        return static_cast<unsigned char> (floor (v * 255.0f + 0.5f));
      }

      void
      string_to_color (const char* string, DvarValue& value)
      {
        float c[4] {};

        if (string != nullptr)
          sscanf (string, "%g %g %g %g", &c[0], &c[1], &c[2], &c[3]);

        value.color[0] = static_cast<char> (color_byte (c[0]));
        value.color[1] = static_cast<char> (color_byte (c[1]));
        value.color[2] = static_cast<char> (color_byte (c[2]));
        value.color[3] = static_cast<char> (color_byte (c[3]));
      }

      DvarValue*
      string_to_value (DvarValue* out,
                       dvarType type,
                       const DvarLimits* domain,
                       const char* string)
      {
        memset (out, 0, sizeof (*out));

        if (string == nullptr)
          string = "";

        switch (type)
        {
          case DVAR_TYPE_BOOL:
            out->enabled = atoi (string) != 0;
            break;

          case DVAR_TYPE_FLOAT:
            out->value = static_cast<float> (atof (string));
            break;

          case DVAR_TYPE_FLOAT_2:
            sscanf (string, "%g %g", &out->vector.x, &out->vector.y);
            break;

          case DVAR_TYPE_FLOAT_3:
          case DVAR_TYPE_FLOAT_3_COLOR:
            if (*string == '(')
              sscanf (string,
                      "( %g, %g, %g )",
                      &out->vector.x,
                      &out->vector.y,
                      &out->vector.z);
            else
              sscanf (string,
                      "%g %g %g",
                      &out->vector.x,
                      &out->vector.y,
                      &out->vector.z);
            break;

          case DVAR_TYPE_FLOAT_4:
            sscanf (string,
                    "%g %g %g %g",
                    &out->vector.x,
                    &out->vector.y,
                    &out->vector.z,
                    &out->vector.w);
            break;

          case DVAR_TYPE_INT:
            out->integer = atoi (string);
            break;

          case DVAR_TYPE_INT64:
            out->integer64 = strtoll (string, nullptr, 10);
            break;

          case DVAR_TYPE_UINT64:
            out->unsignedInt64 = strtoull (string, nullptr, 10);
            break;

          case DVAR_TYPE_ENUM:
            out->integer =
              domain != nullptr ? string_to_enum (*domain, string) : -1337;
            break;

          case DVAR_TYPE_STRING:
            out->string = string;
            break;

          case DVAR_TYPE_COLOR:
            string_to_color (string, *out);
            break;

          default:
            break;
        }

        return out;
      }

      // Thread-local rotating buffer for value_to_string conversions.
      //
      // Here we use four distinct buffers of 256 bytes each to hold up to four
      // concurrent string results within a single expression.
      //
      char*
      next_value_string_buffer ()
      {
        thread_local char buffers[4][256] {};
        thread_local size_t index (0);

        index = (index + 1) % 4;
        buffers[index][0] = '\0';
        return buffers[index];
      }

      int
      clamp_to_int (int64_t value)
      {
        if (value > numeric_limits<int>::max ())
          return numeric_limits<int>::max ();

        if (value < numeric_limits<int>::min ())
          return numeric_limits<int>::min ();

        return static_cast<int> (value);
      }

      int
      clamp_to_int (uint64_t value)
      {
        if (value > static_cast<uint64_t> (numeric_limits<int>::max ()))
          return numeric_limits<int>::max ();

        return static_cast<int> (value);
      }

      // Command argument access wrappers.
      //
      // The cmd_args structure is indexed by nesting level (which
      // ranges from 0 to 7). We must clamp this nesting to the valid range,
      // otherwise we risk undefined behavior whenever our hook is called from
      // unusual execution contexts.

      int
      cmd_argc ()
      {
        int nesting (cmd_args->nesting);
        nesting = max (0, min (7, nesting));
        return cmd_args->argc[nesting];
      }

      const char*
      cmd_argv (int index)
      {
        int nesting (cmd_args->nesting);
        nesting = max (0, min (7, nesting));

        if (index < 0 || index >= cmd_args->argc[nesting])
          return "";

        const char** argv (cmd_args->argv[nesting]);
        return (argv != nullptr && argv[index] != nullptr) ? argv[index] : "";
      }

      void
      get_combined_string (char* out, int start_arg)
      {
        if (out == nullptr)
          return;

        out[0] = '\0';

        const int argc (cmd_argc ());
        int total (0);
        const size_t limit (0x1000);

        for (int i (start_arg); i < argc; ++i)
        {
          const char* arg (cmd_argv (i));
          size_t length (strlen (arg));

          total += static_cast<int> (length) + 1;

          if (total >= 0xFFE)
            break;

          size_t used (strlen (out));
          size_t remaining (limit - used - 1);
          size_t copy_len (min (length, remaining));

          memcpy (out + used, arg, copy_len);
          out[used + copy_len] = '\0';

          if (i != argc - 1 && used + copy_len + 1 < limit)
          {
            out[used + copy_len] = ' ';
            out[used + copy_len + 1] = '\0';
          }
        }
      }
    }

    uint32_t
    hash_dvar_name (const char* name)
    {
      // Dvar mandates a polynomial hash over the lowercased name, using a
      // starting multiplier of 'w' (0x77) that increments per character.
      //
      if (name == nullptr)
        Com_Error (ERR_DROP, "\x15null name in generateHashValue");

      uint32_t hash (0);
      uint32_t multiplier ('w');

      for (const unsigned char* p (
             reinterpret_cast<const unsigned char*> (name));
           *p != '\0';
           ++p)
      {
        hash += static_cast<uint32_t> (tolower (*p)) * multiplier;
        ++multiplier;
      }

      return hash;
    }

    dvar_t*
    find_dvar_raw (const char* name)
    {
      read_lock lock;

      uint32_t bucket (hash_dvar_name (name) & (dvar_hash_buckets - 1));

      for (dvar_t* dvar (dvarHashTable[bucket]); dvar != nullptr;
           dvar = dvar->hashNext)
      {
        if (!tombstoned (dvar) && I_stricmp (name, dvar->name) == 0)
          return dvar;
      }

      return nullptr;
    }

    void
    for_each_dvar (for_each_callback callback, void* user_data)
    {
      if (callback == nullptr)
        return;

      read_lock lock;

      if (!*areDvarsSorted)
        Dvar_Sort ();

      for (int i (0); i < *dvarCount; ++i)
      {
        if (sortedDvars[i] != nullptr && !tombstoned (sortedDvars[i]))
          callback (sortedDvars[i], user_data);
      }
    }

    dvar_t*
    register_variant (const char* name,
                      dvarType type,
                      DvarFlags flags,
                      DvarValue value,
                      DvarLimits domain,
                      const char* description)
    {
      write_lock lock;

      uint32_t bucket (hash_dvar_name (name) & (dvar_hash_buckets - 1));

      // Re-registration flow.
      //
      // If we encounter an existing dvar with the same name, we just need to
      // update its metadata.
      //
      for (dvar_t* existing (dvarHashTable[bucket]); existing != nullptr;
           existing = existing->hashNext)
      {
        if (I_stricmp (name, existing->name) != 0)
          continue;

        set_description (existing, description);

        // We promote DVAR_EXTERNAL string dvars to the declared native type.
        //
        // The context here relate to how the config parser creates
        // DVAR_EXTERNAL string dvars from cfg files way before formal
        // registration happens. When the subsystem registers the dvar with its
        // real type, we must convert the saved config string value to the
        // native type and clear out that DVAR_EXTERNAL flag.
        //
        if ((static_cast<int> (existing->flags) &
             static_cast<int> (DVAR_EXTERNAL)) == 0)
          return existing;

        // Preserve the config string value before we dare modify the type
        // context.
        //
        const char* external_str (nullptr);

        if (existing->type == DVAR_TYPE_STRING &&
            existing->current.string != nullptr)
          external_str = CopyStringInternal (existing->current.string);

        // It is paramount that we free existing string allocations to prevent
        // memory leaks during this promotion.
        //
        if (existing->type == DVAR_TYPE_STRING)
        {
          const char* c (existing->current.string);
          const char* l (existing->latched.string);
          const char* r (existing->reset.string);

          if (c != nullptr)
            FreeStringInternal (c);

          if (l != nullptr && l != c)
            FreeStringInternal (l);

          if (r != nullptr && r != c && r != l)
            FreeStringInternal (r);
        }

        // Apply the new native type, flags, and domain. We must also strip out
        // the DVAR_EXTERNAL flag.
        //
        existing->type = type;
        existing->flags =
          static_cast<DvarFlags> ((static_cast<int> (existing->flags) &
                                   ~static_cast<int> (DVAR_EXTERNAL)) |
                                  static_cast<int> (flags));
        existing->domain = domain;
        existing->modified = false;

        // Set up the reset value using the declared default parameter.
        //
        if (type == DVAR_TYPE_STRING)
        {
          DvarValue reset_val {};
          reset_val.string =
            CopyStringInternal (value.string != nullptr ? value.string : "");
          existing->reset = reset_val;
        }
        else
        {
          copy_value (existing->reset, value, type);
        }

        // Now we parse the preserved config string into the new native type.
        // Should the parsing fail, or the value clearly fall outside the
        // bounds, we fall back to the declared default.
        //
        DvarValue parsed {};

        if (external_str != nullptr)
        {
          string_to_value (&parsed, type, &domain, external_str);

          if (!value_in_domain (type, parsed, domain))
          {
            if (type == DVAR_TYPE_STRING)
              parsed.string = existing->reset.string;
            else
              copy_value (parsed, existing->reset, type);
          }
        }
        else
        {
          if (type == DVAR_TYPE_STRING)
            parsed.string = existing->reset.string;
          else
            copy_value (parsed, existing->reset, type);
        }

        // Apply the resolved current and latched values.
        //
        if (type == DVAR_TYPE_STRING)
        {
          DvarValue str_val {};
          str_val.string =
            CopyStringInternal (parsed.string != nullptr ? parsed.string : "");
          existing->current = str_val;
          existing->latched = str_val;
        }
        else
        {
          copy_value (existing->current, parsed, type);
          copy_value (existing->latched, parsed, type);
        }

        // We can release the saved config string backup now.
        //
        if (external_str != nullptr)
          FreeStringInternal (external_str);

        log::debug << "dvar: promoted external dvar '" << log_safe (name)
                   << "' type=" << static_cast<int> (type)
                   << " flags=" << static_cast<int> (flags);

        return existing;
      }

      // New dvar path.
      //
      // If the dvar does not exist yet, we pull a fresh one right from the
      // pre-allocated pool.
      //
      if (*dvarCount >= static_cast<int> (dvar_pool_capacity))
        Com_Error (ERR_FATAL,
                   "Can't create dvar '%s': %zu dvars already exist",
                   name != nullptr ? name : "",
                   dvar_pool_capacity);

      int index (*dvarCount);
      dvar_t* dvar (&dvarPool[index]);
      dvar_tombstones[index] = false;

      (*dvarCount)++;
      sortedDvars[index] = dvar;
      *areDvarsSorted = false;

      // Keep in mind that the name pointer must persist for the entire lifetime
      // of the process. That is, dvar names typically point directly to string
      // literals. For DVAR_EXTERNAL names coming from the config parser, we
      // make sure to duplicate them into the engine heap.
      //
      dvar->name =
        ((static_cast<int> (flags) & static_cast<int> (DVAR_EXTERNAL)) != 0)
        ? CopyStringInternal (name != nullptr ? name : "")
        : name;

      dvar->flags = flags;
      dvar->type = type;
      dvar->modified = false;
      dvar->pad0[0] = 0;
      dvar->pad0[1] = 0;
      dvar->domain = domain;
      dvar->domainFunc = nullptr;

      if (type == DVAR_TYPE_STRING)
      {
        DvarValue str_val {};
        str_val.string =
          CopyStringInternal (value.string != nullptr ? value.string : "");
        dvar->current = str_val;
        dvar->latched = str_val;
        dvar->reset = str_val;
      }
      else
      {
        copy_value (dvar->current, value, type);
        copy_value (dvar->latched, value, type);
        copy_value (dvar->reset, value, type);
      }

      // Finally, insert the initialized dvar directly into the hash
      // table and record its description.
      //
      {
        uint32_t b (hash_dvar_name (dvar->name) & (dvar_hash_buckets - 1));
        dvar->hashNext = dvarHashTable[b];
        dvarHashTable[b] = dvar;
        set_description (dvar, description);
      }

      log::debug << "dvar: registered '" << log_safe (dvar->name)
                 << "' type=" << static_cast<int> (type)
                 << " flags=" << static_cast<int> (flags);

      return dvar;
    }

    dvar_t*
    register_int64 (const char* name,
                    int64_t value,
                    int64_t min,
                    int64_t max,
                    DvarFlags flags,
                    const char* description)
    {
      // Registering a 64-bit integer requires us to construct a DvarLimits
      // domain that spans the 64-bit range.
      //
      mark_registration_flags (flags);

      DvarValue dvar_value {};
      dvar_value.integer64 = value;

      DvarLimits domain {};
      domain.integer64.min = min;
      domain.integer64.max = max;

      return register_variant (name,
                               DVAR_TYPE_INT64,
                               flags,
                               dvar_value,
                               domain,
                               description);
    }

    dvar_t*
    register_uint64 (const char* name,
                     uint64_t value,
                     uint64_t min,
                     uint64_t max,
                     DvarFlags flags,
                     const char* description)
    {
      // Similar to int64, we need to manually configure the limits boundary.
      //
      mark_registration_flags (flags);

      DvarValue dvar_value {};
      dvar_value.unsignedInt64 = value;

      DvarLimits domain {};
      domain.unsignedInt64.min = min;
      domain.unsignedInt64.max = max;

      return register_variant (name,
                               DVAR_TYPE_UINT64,
                               flags,
                               dvar_value,
                               domain,
                               description);
    }

    void set_variant (dvar_t* dvar, const DvarValue& value, DvarSetSource source);

    const char*
    dvar_description (const dvar_t* dvar)
    {
      return get_description (dvar);
    }

    vector<dvar_snapshot>
    snapshot_dvars ()
    {
      vector<dvar_snapshot> out;

      read_lock lock;

      if (!*areDvarsSorted)
        Dvar_Sort ();

      out.reserve (static_cast<size_t> (*dvarCount));

      for (int i (0); i < *dvarCount; ++i)
      {
        const dvar_t* dvar (sortedDvars[i]);

        if (dvar == nullptr || tombstoned (dvar))
          continue;

        dvar_snapshot s {};
        s.name = dvar->name != nullptr ? dvar->name : "";
        s.type = dvar->type;
        s.flags = dvar->flags;
        s.modified = dvar->modified;
        s.current = dvar->current;
        s.latched = dvar->latched;
        s.reset = dvar->reset;
        s.domain = dvar->domain;
        s.current_text = dvar_value_to_string (dvar, dvar->current);
        s.latched_text = dvar_value_to_string (dvar, dvar->latched);
        s.reset_text = dvar_value_to_string (dvar, dvar->reset);
        s.description = get_description (dvar);

        if (dvar->type == DVAR_TYPE_ENUM &&
            dvar->domain.enumeration.stringCount > 0 &&
            dvar->domain.enumeration.strings != nullptr)
        {
          for (int j (0); j < dvar->domain.enumeration.stringCount; ++j)
          {
            const char* e (dvar->domain.enumeration.strings[j]);
            s.enum_values.emplace_back (e != nullptr ? e : "");
          }
        }

        out.push_back (std::move (s));
      }

      return out;
    }

    dvar_t*
    create_dvar (const dvar_create_info& info)
    {
      if (info.name == nullptr || info.name[0] == '\0')
        return nullptr;

      mark_registration_flags (info.flags);

      DvarValue value (info.value);
      DvarLimits domain (info.domain);

      if (info.type == DVAR_TYPE_ENUM)
      {
        int count (info.enum_value_count);
        if (count <= 0 || info.enum_values == nullptr)
          return nullptr;

        const char** values (new const char*[static_cast<size_t> (count)]);
        for (int i (0); i < count; ++i)
          values[i] = CopyStringInternal (
            info.enum_values[i] != nullptr ? info.enum_values[i] : "");

        domain.enumeration.stringCount = count;
        domain.enumeration.strings = values;

        if (value.integer < 0 || value.integer >= count)
          value.integer = 0;
      }

      const char* name (CopyStringInternal (info.name));

      return register_variant (name,
                               info.type,
                               info.flags,
                               value,
                               domain,
                               info.description);
    }

    bool
    set_dvar_value (const char* name, const DvarValue& value, DvarSetSource src)
    {
      dvar_t* dvar (find_dvar_raw (name));

      if (dvar == nullptr)
        return false;

      set_variant (dvar, value, src);
      return true;
    }

    bool
    set_dvar_reset_value (const char* name, const DvarValue& value)
    {
      dvar_t* dvar (find_dvar_raw (name));

      if (dvar == nullptr || !value_in_domain (dvar->type, value, dvar->domain))
        return false;

      write_lock lock;

      if (dvar->type != DVAR_TYPE_STRING)
      {
        copy_value (dvar->reset, value, dvar->type);
        return true;
      }

      const char* old_reset (dvar->reset.string);
      DvarValue reset {};
      reset.string = CopyStringInternal (value.string != nullptr ? value.string : "");
      dvar->reset = reset;

      if (old_reset != nullptr && old_reset != dvar->current.string &&
          old_reset != dvar->latched.string && old_reset != dvar->reset.string)
        FreeStringInternal (old_reset);

      return true;
    }

    bool
    set_dvar_flags (const char* name, DvarFlags flags)
    {
      dvar_t* dvar (find_dvar_raw (name));

      if (dvar == nullptr)
        return false;

      write_lock lock;
      dvar->flags = flags;
      mark_registration_flags (flags);
      return true;
    }

    bool
    set_dvar_domain (const char* name, const DvarLimits& domain)
    {
      dvar_t* dvar (find_dvar_raw (name));

      if (dvar == nullptr)
        return false;

      write_lock lock;
      dvar->domain = domain;
      return true;
    }

    bool
    set_dvar_description (const char* name, const char* description)
    {
      dvar_t* dvar (find_dvar_raw (name));

      if (dvar == nullptr)
        return false;

      set_description (dvar, description);
      return true;
    }

    bool
    unregister_dvar (const char* name)
    {
      if (name == nullptr || name[0] == '\0')
        return false;

      write_lock lock;

      uint32_t bucket (hash_dvar_name (name) & (dvar_hash_buckets - 1));
      dvar_t* prev (nullptr);

      for (dvar_t* dvar (dvarHashTable[bucket]); dvar != nullptr;
           dvar = dvar->hashNext)
      {
        if (tombstoned (dvar) || I_stricmp (name, dvar->name) != 0)
        {
          prev = dvar;
          continue;
        }

        if (prev != nullptr)
          prev->hashNext = dvar->hashNext;
        else
          dvarHashTable[bucket] = dvar->hashNext;

        dvar->hashNext = nullptr;

        size_t index (0);
        if (pool_index (dvar, index))
          dvar_tombstones[index] = true;

        for (int i (0); i < *dvarCount; ++i)
        {
          if (sortedDvars[i] == dvar)
          {
            for (int j (i); j + 1 < *dvarCount; ++j)
              sortedDvars[j] = sortedDvars[j + 1];

            sortedDvars[*dvarCount - 1] = dvar;
            break;
          }
        }

        log::warning << "dvar: hard-unregistered '" << log_safe (dvar->name)
                     << "'; stale engine references may still exist";
        return true;
      }

      return false;
    }

    void
    add_dvar_flags (dvar_t* dvar, DvarFlags flags)
    {
      if (dvar == nullptr)
        return;

      // We must remember to record these added flags in our TLS tracking slot
      // so the registration accounting remains accurate.
      //
      mark_registration_flags (flags);
      dvar->flags = static_cast<DvarFlags> (static_cast<int> (dvar->flags) |
                                            static_cast<int> (flags));
    }

    bool
    value_in_domain (dvarType type,
                     const DvarValue& value,
                     const DvarLimits& domain)
    {
      // Validates whether a given value fits within the bounds established by
      // the dvar's domain constraints.
      //
      switch (type)
      {
        case DVAR_TYPE_BOOL:
        case DVAR_TYPE_STRING:
        case DVAR_TYPE_COLOR:
          // These types are unconstrained by arbitrary bounds.
          //
          return true;

        case DVAR_TYPE_FLOAT:
          // We use inverted greater-than logic here to catch and reject NaN
          // values.
          //
          return !(domain.value.min > value.value) &&
                 !(value.value > domain.value.max);

        case DVAR_TYPE_FLOAT_2:
          for (int i (0); i < 2; ++i)
          {
            if (domain.vector.min > value.vector.value[i] ||
                value.vector.value[i] > domain.vector.max)
              return false;
          }
          return true;

        case DVAR_TYPE_FLOAT_3:
          for (int i (0); i < 3; ++i)
          {
            if (domain.vector.min > value.vector.value[i] ||
                value.vector.value[i] > domain.vector.max)
              return false;
          }
          return true;

        case DVAR_TYPE_FLOAT_4:
          for (int i (0); i < 4; ++i)
          {
            if (domain.vector.min > value.vector.value[i] ||
                value.vector.value[i] > domain.vector.max)
              return false;
          }
          return true;

        case DVAR_TYPE_INT:
          return value.integer >= domain.integer.min &&
                 value.integer <= domain.integer.max;

        case DVAR_TYPE_INT64:
          return value.integer64 >= domain.integer64.min &&
                 value.integer64 <= domain.integer64.max;

        case DVAR_TYPE_UINT64:
          return value.unsignedInt64 >= domain.unsignedInt64.min &&
                 value.unsignedInt64 <= domain.unsignedInt64.max;

        case DVAR_TYPE_ENUM:
          // Keep in mind that an index of 0 is always accepted. Dvar uses this
          // as a safe default when the domain has at least one entry but the
          // provided value is otherwise invalid.
          //
          return (value.integer >= 0 &&
                  value.integer < domain.enumeration.stringCount) ||
                 value.integer == 0;

        case DVAR_TYPE_FLOAT_3_COLOR:
          for (int i (0); i < 3; ++i)
          {
            if (0.0f > value.vector.value[i] ||
                value.vector.value[i] > domain.vector.max)
              return false;
          }
          return true;

        default:
          return false;
      }
    }

    bool
    values_equal (dvarType type, const DvarValue& lhs, const DvarValue& rhs)
    {
      switch (type)
      {
        case DVAR_TYPE_BOOL:
          return lhs.enabled == rhs.enabled;

        case DVAR_TYPE_FLOAT:
          return lhs.value == rhs.value;

        case DVAR_TYPE_FLOAT_2:
          return lhs.vector.x == rhs.vector.x && lhs.vector.y == rhs.vector.y;

        case DVAR_TYPE_FLOAT_3:
        case DVAR_TYPE_FLOAT_3_COLOR:
          return lhs.vector.x == rhs.vector.x && lhs.vector.y == rhs.vector.y &&
                 lhs.vector.z == rhs.vector.z;

        case DVAR_TYPE_FLOAT_4:
          return lhs.vector.x == rhs.vector.x && lhs.vector.y == rhs.vector.y &&
                 lhs.vector.z == rhs.vector.z && lhs.vector.w == rhs.vector.w;

        case DVAR_TYPE_INT:
        case DVAR_TYPE_ENUM:
          return lhs.integer == rhs.integer;

        case DVAR_TYPE_INT64:
          return lhs.integer64 == rhs.integer64;

        case DVAR_TYPE_UINT64:
          return lhs.unsignedInt64 == rhs.unsignedInt64;

        case DVAR_TYPE_STRING:
          // We must compare contents, not just the pointers here. Dvar may have
          // multiple distinct copies of the exact same string scattered across
          // different allocation slots.
          //
          if (lhs.string == rhs.string)
            return true;

          if (lhs.string == nullptr || rhs.string == nullptr)
            return false;

          return strcmp (lhs.string, rhs.string) == 0;

        case DVAR_TYPE_COLOR:
          return memcmp (lhs.color, rhs.color, sizeof (lhs.color)) == 0;

        default:
          return false;
      }
    }

    void
    set_latched_value (dvar_t* dvar, const DvarValue& value)
    {
      if (dvar == nullptr)
        return;

      // If it's not a string, we can just do a straightforward value copy and
      // bail out.
      //
      if (dvar->type != DVAR_TYPE_STRING)
      {
        copy_value (dvar->latched, value, dvar->type);
        return;
      }

      const char* old_latched (dvar->latched.string);

      if (old_latched == value.string)
        return;

      // We need to verify if the old latched string is exclusive to this slot.
      // If it's shared with current or reset, we shouldn't touch it to avoid
      // dangling pointers.
      //
      bool free_old (old_latched != nullptr &&
                     old_latched != dvar->current.string &&
                     old_latched != dvar->reset.string);

      DvarValue latched (canonical_string_value (dvar, value.string));
      dvar->latched = latched;

      if (free_old)
        FreeStringInternal (old_latched);
    }

    namespace
    {
      // Writes to dvar->current (and, for non-latching types, dvar->latched).
      //
      // We separate this from set_variant() because it possesses
      // different string-ownership semantics depending on whether a
      // latching mechanism is concurrent.
      //
      void
      assign_current_value (dvar_t* dvar, const DvarValue& value)
      {
        switch (dvar->type)
        {
          case DVAR_TYPE_FLOAT_2:
          case DVAR_TYPE_FLOAT_3:
          case DVAR_TYPE_FLOAT_3_COLOR:
          case DVAR_TYPE_FLOAT_4:
            copy_value (dvar->current, value, dvar->type);
            copy_value (dvar->latched, value, dvar->type);
            break;

          case DVAR_TYPE_STRING:
          {
            const char* old_current (dvar->current.string);
            bool free_old_current (old_current != nullptr &&
                                   old_current != dvar->latched.string &&
                                   old_current != dvar->reset.string);

            DvarValue current (
              canonical_current_string_value (dvar, value.string));

            const char* old_latched (dvar->latched.string);
            dvar->current = current;

            // Purge the old latched value if it just got superseded and isn't
            // pinned by another slot.
            //
            if (old_latched != nullptr && old_latched != dvar->current.string &&
                old_latched != dvar->reset.string)
              FreeStringInternal (old_latched);

            dvar->latched = current;

            if (free_old_current)
              FreeStringInternal (old_current);

            break;
          }

          default:
            copy_value (dvar->current, value, dvar->type);
            copy_value (dvar->latched, value, dvar->type);
            break;
        }

        dvar->modified = true;
      }
    }

    void
    set_variant (dvar_t* dvar, const DvarValue& value, DvarSetSource source)
    {
      if (dvar == nullptr)
        return;

      // Call the engine's pre-set hook. It lives at a fixed address and acts as
      // an invariant enforcer right before any value mutation takes place.
      //
      reinterpret_cast<void (*) ()> (pre_set_variant_fn) ();

      DvarValue value_to_set (value);

      // Clamp or reject out-of-domain values. For enums, the convention is to
      // fall back to the reset value rather than outright rejecting the set
      // operation.
      //
      if (!value_in_domain (dvar->type, value_to_set, dvar->domain))
      {
        if (dvar->type != DVAR_TYPE_ENUM)
          return;

        value_to_set = dvar->reset;
      }

      // Execute the domain function callback if one happens to be installed.
      //
      // Keep in mind the callback returns false if it wants to veto the set. We
      // guard against invoking non-executable callbacks (which sometimes occur
      // if the callback pointer is raw garbage inherited from a stale engine
      // reload state).
      //
      if (dvar->domainFunc != nullptr)
      {
        const void* cb (reinterpret_cast<const void*> (dvar->domainFunc));

        if (!executable_page (cb))
        {
          log::warning << "dvar: skipped non-executable domain callback for '"
                       << log_safe (dvar->name) << "' callback=" << cb;
          return;
        }

        DvarValue cb_value (value_to_set);

        if (!dvar->domainFunc (dvar, &cb_value))
          return;
      }

      // Enforce stringent write restrictions for external and script sources.
      //
      if (source == DVAR_SOURCE_EXTERNAL || source == DVAR_SOURCE_SCRIPT)
      {
        int flags (static_cast<int> (dvar->flags));

        // Read-only and init-only dvars cannot be touched from these sources.
        //
        if ((flags &
             (static_cast<int> (DVAR_INIT) | static_cast<int> (DVAR_ROM))) != 0)
          return;

        // Ensure players can't toggle cheat dvars via the console unless
        // cheats_enabled_flag allows it.
        //
        if (source == DVAR_SOURCE_EXTERNAL &&
            (flags & static_cast<int> (DVAR_CHEAT)) != 0 &&
            *reinterpret_cast<unsigned char*> (cheats_enabled_flag) == 0)
          return;

        if ((flags & static_cast<int> (DVAR_LATCH)) != 0)
        {
          set_latched_value (dvar, value_to_set);
          return;
        }
      }

      mark_modified_flags (dvar);

      // If the incoming value hasn't changed, we can just synchronize the
      // latched slot without enduring a full assignment cycle.
      //
      if (values_equal (dvar->type, dvar->current, value_to_set))
      {
        set_latched_value (dvar, dvar->current);
        return;
      }

      assign_current_value (dvar, value_to_set);
    }

    void
    set_dvar_raw_int64 (dvar_t* dvar, int64_t value, DvarSetSource source)
    {
      DvarValue v {};
      v.integer64 = value;
      set_variant (dvar, v, source);
    }

    void
    set_dvar_raw_uint64 (dvar_t* dvar, uint64_t value, DvarSetSource source)
    {
      DvarValue v {};
      v.unsignedInt64 = value;
      set_variant (dvar, v, source);
    }

    int64_t
    get_raw_int64 (const dvar_t* dvar)
    {
      if (dvar == nullptr)
        return 0;

      switch (dvar->type)
      {
        case DVAR_TYPE_BOOL:
          return dvar->current.enabled ? 1 : 0;
        case DVAR_TYPE_FLOAT:
          return static_cast<int64_t> (dvar->current.value);
        case DVAR_TYPE_INT:
        case DVAR_TYPE_ENUM:
          return dvar->current.integer;
        case DVAR_TYPE_INT64:
          return dvar->current.integer64;
        case DVAR_TYPE_UINT64:
          // We have to clamp this as we are casting an unsigned 64-bit value
          // down into a signed domain.
          //
          if (dvar->current.unsignedInt64 >
              static_cast<uint64_t> (numeric_limits<int64_t>::max ()))
            return numeric_limits<int64_t>::max ();
          return static_cast<int64_t> (dvar->current.unsignedInt64);
        case DVAR_TYPE_STRING:
          return strtoll (dvar->current.string != nullptr ? dvar->current.string
                                                          : "",
                          nullptr,
                          10);
        default:
          return 0;
      }
    }

    uint64_t
    get_raw_uint64 (const dvar_t* dvar)
    {
      if (dvar == nullptr)
        return 0;

      switch (dvar->type)
      {
        case DVAR_TYPE_BOOL:
          return dvar->current.enabled ? 1ULL : 0ULL;
        case DVAR_TYPE_FLOAT:
          return dvar->current.value < 0.0f
            ? 0ULL
            : static_cast<uint64_t> (dvar->current.value);
        case DVAR_TYPE_INT:
        case DVAR_TYPE_ENUM:
          return dvar->current.integer < 0
            ? 0ULL
            : static_cast<uint64_t> (dvar->current.integer);
        case DVAR_TYPE_INT64:
          return dvar->current.integer64 < 0
            ? 0ULL
            : static_cast<uint64_t> (dvar->current.integer64);
        case DVAR_TYPE_UINT64:
          return dvar->current.unsignedInt64;
        case DVAR_TYPE_STRING:
          return strtoull (
            dvar->current.string != nullptr ? dvar->current.string : "",
            nullptr,
            10);
        default:
          return 0;
      }
    }

    const char*
    dvar_value_to_string (const dvar_t* dvar, const DvarValue& value)
    {
      if (dvar == nullptr)
        return "";

      // Notice that we pull from a thread-local rotating buffer pool here so we
      // can support multiple string conversions within a single printf.
      //
      char* buf (next_value_string_buffer ());

      switch (dvar->type)
      {
        case DVAR_TYPE_BOOL:
          return value.enabled ? "1" : "0";

        case DVAR_TYPE_FLOAT:
          snprintf (buf, 256, "%g", static_cast<double> (value.value));
          return buf;

        case DVAR_TYPE_FLOAT_2:
          snprintf (buf,
                    256,
                    "%g %g",
                    static_cast<double> (value.vector.x),
                    static_cast<double> (value.vector.y));
          return buf;

        case DVAR_TYPE_FLOAT_3:
        case DVAR_TYPE_FLOAT_3_COLOR:
          snprintf (buf,
                    256,
                    "%g %g %g",
                    static_cast<double> (value.vector.x),
                    static_cast<double> (value.vector.y),
                    static_cast<double> (value.vector.z));
          return buf;

        case DVAR_TYPE_FLOAT_4:
          snprintf (buf,
                    256,
                    "%g %g %g %g",
                    static_cast<double> (value.vector.x),
                    static_cast<double> (value.vector.y),
                    static_cast<double> (value.vector.z),
                    static_cast<double> (value.vector.w));
          return buf;

        case DVAR_TYPE_INT:
          snprintf (buf, 256, "%i", value.integer);
          return buf;

        case DVAR_TYPE_INT64:
          snprintf (buf, 256, "%" PRId64, value.integer64);
          return buf;

        case DVAR_TYPE_UINT64:
          snprintf (buf, 256, "%" PRIu64, value.unsignedInt64);
          return buf;

        case DVAR_TYPE_ENUM:
          // Make sure we don't read out of bounds on the enum list array.
          //
          if (dvar->domain.enumeration.stringCount > 0 && value.integer >= 0 &&
              value.integer < dvar->domain.enumeration.stringCount)
            return dvar->domain.enumeration.strings[value.integer];
          return "";

        case DVAR_TYPE_STRING:
          return value.string != nullptr ? value.string : "";

        case DVAR_TYPE_COLOR:
          snprintf (
            buf,
            256,
            "%g %g %g %g",
            static_cast<double> (static_cast<unsigned char> (value.color[0]) *
                                 (1.0f / 255.0f)),
            static_cast<double> (static_cast<unsigned char> (value.color[1]) *
                                 (1.0f / 255.0f)),
            static_cast<double> (static_cast<unsigned char> (value.color[2]) *
                                 (1.0f / 255.0f)),
            static_cast<double> (static_cast<unsigned char> (value.color[3]) *
                                 (1.0f / 255.0f)));
          return buf;

        default:
          return "";
      }
    }

    namespace
    {
      void
      set_bool (dvar_t* dvar, bool value, DvarSetSource source)
      {
        if (dvar == nullptr)
          return;

        DvarValue v {};

        if (dvar->type == DVAR_TYPE_BOOL)
          v.enabled = value;
        else
          v.string = value ? "1" : "0";

        set_variant (dvar, v, source);
      }

      void
      set_int (dvar_t* dvar, int value, DvarSetSource source)
      {
        if (dvar == nullptr)
          return;

        DvarValue v {};
        char buf[32] {};

        if (dvar->type == DVAR_TYPE_INT || dvar->type == DVAR_TYPE_ENUM)
          v.integer = value;
        else if (dvar->type == DVAR_TYPE_INT64)
          v.integer64 = value;
        else if (dvar->type == DVAR_TYPE_UINT64)
          v.unsignedInt64 = value < 0 ? 0 : static_cast<uint64_t> (value);
        else
        {
          snprintf (buf, sizeof (buf), "%i", value);
          v.string = buf;
        }

        set_variant (dvar, v, source);
      }

      void
      set_float (dvar_t* dvar, float value, DvarSetSource source)
      {
        if (dvar == nullptr)
          return;

        DvarValue v {};
        char buf[64] {};

        if (dvar->type == DVAR_TYPE_FLOAT)
          v.value = value;
        else
        {
          snprintf (buf, sizeof (buf), "%g", static_cast<double> (value));
          v.string = buf;
        }

        set_variant (dvar, v, source);
      }

      void
      set_string (dvar_t* dvar, const char* value, DvarSetSource source)
      {
        if (dvar == nullptr)
          return;

        DvarValue v {};

        if (dvar->type == DVAR_TYPE_STRING)
          v.string = value != nullptr ? value : "";
        else if (dvar->type == DVAR_TYPE_ENUM)
          v.integer = string_to_enum (dvar->domain, value);
        else
          string_to_value (&v, dvar->type, &dvar->domain, value);

        set_variant (dvar, v, source);
      }

      void
      update_reset_value (dvar_t* dvar, const DvarValue& value)
      {
        if (dvar == nullptr)
          return;

        if (dvar->type != DVAR_TYPE_STRING)
        {
          copy_value (dvar->reset, value, dvar->type);
          return;
        }

        const char* old_reset (dvar->reset.string);
        DvarValue reset (canonical_reset_string_value (dvar, value.string));

        dvar->reset = reset;

        // Be sure we aren't leaking the old string, or worst, freeing a string
        // that current or latched are still using.
        //
        if (old_reset != nullptr && old_reset != dvar->current.string &&
            old_reset != dvar->latched.string &&
            old_reset != dvar->reset.string)
          FreeStringInternal (old_reset);
      }

      void
      reset (dvar_t* dvar, DvarSetSource source)
      {
        if (dvar == nullptr)
          return;

        DvarValue value (dvar->reset);
        set_variant (dvar, value, source);
      }

      void
      reset_script_info ()
      {
        write_lock lock;

        for (int i (0); i < *dvarCount; ++i)
        {
          dvarPool[i].flags =
            static_cast<DvarFlags> (static_cast<int> (dvarPool[i].flags) &
                                    ~static_cast<int> (DVAR_SCRIPTINFO));
        }
      }

      bool
      get_bool (const char* name)
      {
        dvar_t* dvar (find_dvar_raw (name));

        if (dvar == nullptr)
          return false;

        switch (dvar->type)
        {
          case DVAR_TYPE_BOOL:
            return dvar->current.enabled;
          case DVAR_TYPE_INT:
          case DVAR_TYPE_ENUM:
            return dvar->current.integer != 0;
          case DVAR_TYPE_INT64:
            return dvar->current.integer64 != 0;
          case DVAR_TYPE_UINT64:
            return dvar->current.unsignedInt64 != 0;
          case DVAR_TYPE_FLOAT:
            return dvar->current.value != 0.0f;
          case DVAR_TYPE_STRING:
            return atoi (dvar->current.string != nullptr ? dvar->current.string
                                                         : "") != 0;
          default:
            return false;
        }
      }

      float
      get_float (const char* name)
      {
        dvar_t* dvar (find_dvar_raw (name));

        if (dvar == nullptr)
          return 0.0f;

        switch (dvar->type)
        {
          case DVAR_TYPE_FLOAT:
            return dvar->current.value;
          case DVAR_TYPE_BOOL:
            return dvar->current.enabled ? 1.0f : 0.0f;
          case DVAR_TYPE_INT:
          case DVAR_TYPE_ENUM:
            return static_cast<float> (dvar->current.integer);
          case DVAR_TYPE_INT64:
            return static_cast<float> (dvar->current.integer64);
          case DVAR_TYPE_UINT64:
            return static_cast<float> (dvar->current.unsignedInt64);
          case DVAR_TYPE_STRING:
            return static_cast<float> (atof (
              dvar->current.string != nullptr ? dvar->current.string : ""));
          default:
            return 0.0f;
        }
      }

      void
      set_command (const char* name, const char* value)
      {
        dvar_t* dvar (find_dvar_raw (name));

        // If the targeted dvar doesn't exist yet, we register it as a
        // DVAR_EXTERNAL string dvar so the engine knows it originated from a
        // config or console command.
        //
        if (dvar == nullptr)
        {
          dvar = Dvar_RegisterString (name,
                                      value != nullptr ? value : "",
                                      DVAR_EXTERNAL,
                                      "External Dvar");
        }
        else
        {
          set_string (dvar, value, DVAR_SOURCE_EXTERNAL);
        }

        if (dvar == nullptr)
          return;

        // If we are parsing autoexec configs, make sure we stamp the autoexec
        // flag onto the dvar and update its reset state to match.
        //
        if (*reinterpret_cast<unsigned char*> (dvar_autoexec_flag) != 0)
        {
          add_dvar_flags (dvar, DVAR_AUTOEXEC);
          update_reset_value (dvar, dvar->current);
        }
      }

      dvar_t*
      set_from_string_by_name (const char* name, const char* value)
      {
        dvar_t* dvar (find_dvar_raw (name));

        if (dvar == nullptr)
          return Dvar_RegisterString (name,
                                      value != nullptr ? value : "",
                                      DVAR_EXTERNAL,
                                      "External Dvar");

        set_string (dvar, value, DVAR_SOURCE_INTERNAL);
        return dvar;
      }

      void
      do_toggle_internal ()
      {
        // Toggle behaviour supports two distinct modes:
        //
        //  toggle <name>           binary flip (bool) or cycle (enum/int)
        //  toggle <name> v1 v2 ... advance (cycle) through the listed values

        if (cmd_argc () < 2)
          return;

        const char* name (cmd_argv (1));
        dvar_t* dvar (find_dvar_raw (name));

        if (dvar == nullptr)
          return;

        if (cmd_argc () == 2)
        {
          DvarValue value {};

          switch (dvar->type)
          {
            case DVAR_TYPE_BOOL:
              value.enabled = !dvar->current.enabled;
              set_variant (dvar, value, DVAR_SOURCE_EXTERNAL);
              return;

            case DVAR_TYPE_INT:
            case DVAR_TYPE_ENUM:
              if (dvar->type == DVAR_TYPE_ENUM)
              {
                if (dvar->domain.enumeration.stringCount == 0)
                  return;

                value.integer = (dvar->current.integer + 1) %
                  dvar->domain.enumeration.stringCount;
              }
              else if (dvar->domain.integer.min <= 0 &&
                       dvar->domain.integer.max >= 1)
              {
                value.integer = dvar->current.integer == 0 ? 1 : 0;
              }
              else
              {
                value.integer =
                  dvar->current.integer == dvar->domain.integer.min
                  ? dvar->domain.integer.max
                  : dvar->domain.integer.min;
              }

              set_variant (dvar, value, DVAR_SOURCE_EXTERNAL);
              return;

            case DVAR_TYPE_INT64:
              value.integer64 =
                dvar->current.integer64 == dvar->domain.integer64.min
                ? dvar->domain.integer64.max
                : dvar->domain.integer64.min;
              set_variant (dvar, value, DVAR_SOURCE_EXTERNAL);
              return;

            case DVAR_TYPE_UINT64:
              value.unsignedInt64 =
                dvar->current.unsignedInt64 == dvar->domain.unsignedInt64.min
                ? dvar->domain.unsignedInt64.max
                : dvar->domain.unsignedInt64.min;
              set_variant (dvar, value, DVAR_SOURCE_EXTERNAL);
              return;

            case DVAR_TYPE_FLOAT:
              if (dvar->domain.value.min <= 0.0f &&
                  dvar->domain.value.max >= 1.0f)
                value.value = dvar->current.value == 0.0f ? 1.0f : 0.0f;
              else
                value.value = dvar->current.value == dvar->domain.value.min
                  ? dvar->domain.value.max
                  : dvar->domain.value.min;

              set_variant (dvar, value, DVAR_SOURCE_EXTERNAL);
              return;

            default:
              return;
          }
        }

        // Multi-value toggle passed by the user. Cycle through the listed
        // arguments.
        //
        const char* current (dvar_value_to_string (dvar, dvar->current));
        const char* next (cmd_argv (2));

        for (int i (2); i < cmd_argc (); ++i)
        {
          const char* candidate (cmd_argv (i));

          if (dvar->type == DVAR_TYPE_ENUM)
          {
            int idx (string_to_enum (dvar->domain, candidate));

            if (idx >= 0 && idx < dvar->domain.enumeration.stringCount)
              candidate = dvar->domain.enumeration.strings[idx];
          }

          if (I_stricmp (current, candidate) == 0)
          {
            next = (i + 1 < cmd_argc ()) ? cmd_argv (i + 1) : cmd_argv (2);
            break;
          }
        }

        set_command (name, next);
      }
    }

    namespace
    {
      extern "C"
      {
        dvar_t*
        dvar_find_var_hook (const char* name)
        {
          return find_dvar_raw (name);
        }

        void
        dvar_for_each_hook (for_each_callback callback, void* user_data)
        {
          for_each_dvar (callback, user_data);
        }

        dvar_t*
        dvar_register_variant_hook (const char* name,
                                    dvarType type,
                                    DvarFlags flags,
                                    const DvarValue* value,
                                    const DvarLimits* domain,
                                    const char* description)
        {
          log::debug << "dvar: register_variant '" << log_safe (name)
                     << "' type=" << static_cast<int> (type)
                     << " flags=" << static_cast<int> (flags);

          DvarValue local_value {};
          DvarLimits local_domain {};

          if (value != nullptr)
            local_value = *value;
          if (domain != nullptr)
            local_domain = *domain;

          return register_variant (name,
                                   type,
                                   flags,
                                   local_value,
                                   local_domain,
                                   description);
        }

        dvar_t*
        dvar_register_int_hook (const char* name,
                                int value,
                                int min,
                                int max,
                                DvarFlags flags,
                                const char* description)
        {
          log::debug << "dvar: register_int '" << log_safe (name) << "'";

          mark_registration_flags (flags);

          DvarValue v {};
          DvarLimits d {};
          v.integer = value;
          d.integer.min = min;
          d.integer.max = max;

          return register_variant (name,
                                   DVAR_TYPE_INT,
                                   flags,
                                   v,
                                   d,
                                   description);
        }

        dvar_t*
        dvar_register_bool_hook (const char* name,
                                 bool value,
                                 DvarFlags flags,
                                 const char* description)
        {
          log::debug << "dvar: register_bool '" << log_safe (name) << "'";
          dvar_t* dvar (Dvar_RegisterBool (name, value, flags, description));
          set_description (dvar, description);
          return dvar;
        }

        dvar_t*
        dvar_register_float_hook (const char* name,
                                  float value,
                                  float min,
                                  float max,
                                  DvarFlags flags,
                                  const char* description)
        {
          log::debug << "dvar: register_float '" << log_safe (name) << "'";
          dvar_t* dvar (
            Dvar_RegisterFloat (name, value, min, max, flags, description));
          set_description (dvar, description);
          return dvar;
        }

        dvar_t*
        dvar_register_string_hook (const char* name,
                                   const char* value,
                                   DvarFlags flags,
                                   const char* description)
        {
          log::debug << "dvar: register_string '" << log_safe (name)
                     << "' value='" << log_safe (value) << "'";
          dvar_t* dvar (Dvar_RegisterString (name, value, flags, description));
          set_description (dvar, description);
          return dvar;
        }

        dvar_t*
        dvar_register_enum_hook (const char* name,
                                 const char** value_list,
                                 int default_index,
                                 DvarFlags flags,
                                 const char* description)
        {
          log::debug << "dvar: register_enum '" << log_safe (name) << "'";
          dvar_t* dvar (Dvar_RegisterEnum (name,
                                           value_list,
                                           default_index,
                                           flags,
                                           description));
          set_description (dvar, description);
          return dvar;
        }

        dvar_t*
        dvar_register_color_hook (const char* name,
                                  float r,
                                  float g,
                                  float b,
                                  float a,
                                  DvarFlags flags,
                                  const char* description)
        {
          log::debug << "dvar: register_color '" << log_safe (name) << "'";
          dvar_t* dvar (
            Dvar_RegisterColor (name, r, g, b, a, flags, description));
          set_description (dvar, description);
          return dvar;
        }

        dvar_t*
        dvar_register_vec2_hook (const char* name,
                                 float x,
                                 float y,
                                 float min,
                                 float max,
                                 DvarFlags flags,
                                 const char* description)
        {
          log::debug << "dvar: register_vec2 '" << log_safe (name) << "'";
          dvar_t* dvar (
            Dvar_RegisterVec2 (name, x, y, min, max, flags, description));
          set_description (dvar, description);
          return dvar;
        }

        dvar_t*
        dvar_register_vec3_hook (const char* name,
                                 float x,
                                 float y,
                                 float z,
                                 float min,
                                 float max,
                                 DvarFlags flags,
                                 const char* description)
        {
          log::debug << "dvar: register_vec3 '" << log_safe (name) << "'";
          dvar_t* dvar (
            Dvar_RegisterVec3 (name, x, y, z, min, max, flags, description));
          set_description (dvar, description);
          return dvar;
        }

        dvar_t*
        dvar_register_vec3_color_hook (const char* name,
                                       float r,
                                       float g,
                                       float b,
                                       DvarFlags flags,
                                       const char* description)
        {
          log::debug << "dvar: register_vec3_color '" << log_safe (name) << "'";
          dvar_t* dvar (
            Dvar_RegisterVec3Color (name, r, g, b, flags, description));
          set_description (dvar, description);
          return dvar;
        }

        dvar_t*
        dvar_register_vec4_hook (const char* name,
                                 float x,
                                 float y,
                                 float z,
                                 float w,
                                 float min,
                                 float max,
                                 DvarFlags flags,
                                 const char* description)
        {
          log::debug << "dvar: register_vec4 '" << log_safe (name) << "'";
          dvar_t* dvar (
            Dvar_RegisterVec4 (name, x, y, z, w, min, max, flags, description));
          set_description (dvar, description);
          return dvar;
        }

        bool
        dvar_value_in_domain_hook (dvarType type,
                                   DvarValue value,
                                   DvarLimits domain)
        {
          return value_in_domain (type, value, domain);
        }

        bool
        dvar_values_equal_hook (dvarType type, DvarValue lhs, DvarValue rhs)
        {
          return values_equal (type, lhs, rhs);
        }

        void
        dvar_set_latched_value_hook (dvar_t* dvar, DvarValue* value)
        {
          if (value != nullptr)
            set_latched_value (dvar, *value);
        }

        int
        dvar_string_to_enum_hook (DvarLimits domain, const char* string)
        {
          return string_to_enum (domain, string);
        }

        DvarValue*
        dvar_string_to_value_hook (DvarValue* out,
                                   dvarType type,
                                   DvarLimits* domain,
                                   const char* string)
        {
          return string_to_value (out, type, domain, string);
        }

        void
        dvar_string_to_color_hook (const char* string, DvarValue* value)
        {
          if (value != nullptr)
            string_to_color (string, *value);
        }

        void
        dvar_get_combined_string_hook (char* out, int start_arg)
        {
          get_combined_string (out, start_arg);
        }

        void
        dvar_add_flags_hook (dvar_t* dvar, DvarFlags flags)
        {
          add_dvar_flags (dvar, flags);
        }

        void
        dvar_reset_hook (dvar_t* dvar, DvarSetSource source)
        {
          reset (dvar, source);
        }

        void
        dvar_reset_script_info_hook ()
        {
          reset_script_info ();
        }

        void
        dvar_update_reset_value_hook (dvar_t* dvar, DvarValue value)
        {
          update_reset_value (dvar, value);
        }

        bool
        dvar_get_bool_hook (const char* name)
        {
          return get_bool (name);
        }

        float
        dvar_get_float_hook (const char* name)
        {
          return get_float (name);
        }

        int
        dvar_get_int_by_name_hook (const char* name)
        {
          dvar_t* dvar (find_dvar_raw (name));

          if (dvar == nullptr)
            return 0;

          return clamp_to_int (get_raw_int64 (dvar));
        }

        const char*
        dvar_get_string_hook (const char* name)
        {
          dvar_t* dvar (find_dvar_raw (name));

          if (dvar == nullptr)
            return "";

          return dvar_value_to_string (dvar, dvar->current);
        }

        const char*
        dvar_get_variant_string_hook (const char* name)
        {
          dvar_t* dvar (find_dvar_raw (name));

          if (dvar == nullptr)
            return "";

          return dvar_value_to_string (dvar, dvar->current);
        }

        void
        dvar_set_bool_hook (dvar_t* dvar, bool value)
        {
          set_bool (dvar, value, DVAR_SOURCE_INTERNAL);
        }

        void
        dvar_set_bool_by_name_hook (const char* name, bool value)
        {
          dvar_t* dvar (find_dvar_raw (name));
          set_bool (dvar, value, DVAR_SOURCE_INTERNAL);
        }

        void
        dvar_set_float_hook (dvar_t* dvar, float value)
        {
          set_float (dvar, value, DVAR_SOURCE_INTERNAL);
        }

        void
        dvar_set_int_hook (dvar_t* dvar, int value)
        {
          set_int (dvar, value, DVAR_SOURCE_INTERNAL);
        }

        void
        dvar_set_int_by_name_hook (const char* name, int value)
        {
          dvar_t* dvar (find_dvar_raw (name));
          set_int (dvar, value, DVAR_SOURCE_INTERNAL);
        }

        void
        dvar_set_string_hook (dvar_t* dvar, const char* value)
        {
          set_string (dvar, value, DVAR_SOURCE_INTERNAL);
        }

        void
        dvar_set_string_by_name_hook (const char* name, const char* value)
        {
          dvar_t* dvar (find_dvar_raw (name));
          set_string (dvar, value, DVAR_SOURCE_INTERNAL);
        }

        dvar_t*
        dvar_set_from_string_by_name_hook (const char* name, const char* value)
        {
          return set_from_string_by_name (name, value);
        }

        void
        dvar_set_from_string_from_source_hook (dvar_t* dvar,
                                               const char* value,
                                               DvarSetSource source)
        {
          set_string (dvar, value, source);
        }

        void
        dvar_set_command_hook (const char* name, const char* value)
        {
          set_command (name, value);
        }

        const char*
        dvar_value_to_string_hook (dvar_t* dvar, DvarValue value)
        {
          return dvar_value_to_string (dvar, value);
        }

        const char*
        dvar_displayable_value_hook (dvar_t* dvar)
        {
          if (dvar == nullptr)
            return "";

          return dvar_value_to_string (dvar, dvar->current);
        }

        int
        dvar_command_hook ()
        {
          const char* name (cmd_argv (0));
          dvar_t* dvar (find_dvar_raw (name));

          if (dvar == nullptr)
            return 0;

          if (cmd_argc () != 1)
          {
            char combined[0x1000] {};
            get_combined_string (combined, 1);
            set_command (name, combined);
          }

          return 1;
        }

        void
        dvar_reset_f_hook ()
        {
          if (cmd_argc () != 2)
            return;

          dvar_t* dvar (find_dvar_raw (cmd_argv (1)));

          if (dvar != nullptr)
            reset (dvar, DVAR_SOURCE_EXTERNAL);
        }

        void
        dvar_set_f_hook ()
        {
          if (cmd_argc () < 3)
            return;

          const char* name (cmd_argv (1));

          if (!Dvar_IsValidName (name))
            return;

          char combined[0x1000] {};
          get_combined_string (combined, 2);
          set_command (name, combined);
        }

        void
        dvar_seta_f_hook ()
        {
          if (cmd_argc () < 3)
            return;

          const char* name (cmd_argv (1));

          if (!Dvar_IsValidName (name))
            return;

          char combined[0x1000] {};
          get_combined_string (combined, 2);
          set_command (name, combined);

          dvar_t* dvar (find_dvar_raw (name));

          if (dvar != nullptr)
            add_dvar_flags (dvar, DVAR_ARCHIVE);
        }

        void
        dvar_set_from_dvar_f_hook ()
        {
          if (cmd_argc () != 3)
            return;

          dvar_t* source (find_dvar_raw (cmd_argv (2)));

          if (source == nullptr)
            return;

          set_command (cmd_argv (1),
                       dvar_value_to_string (source, source->current));
        }

        bool
        dvar_toggle_internal_hook ()
        {
          if (cmd_argc () < 2)
            return false;

          const char* name (cmd_argv (1));
          dvar_t* dvar (find_dvar_raw (name));

          if (dvar == nullptr)
            return false;

          do_toggle_internal ();
          return true;
        }

        void
        dvar_toggle_print_f_hook ()
        {
          (void) dvar_toggle_internal_hook ();
        }

        void
        dvar_toggle_f_hook ()
        {
          (void) dvar_toggle_internal_hook ();
        }

        void
        dvar_add_commands_hook ()
        {
          Cmd_AddCommandInternal (
            "toggle",
            &dvar_toggle_f_hook,
            reinterpret_cast<cmd_function_s*> (0x141C98ED0));
          Cmd_AddCommandInternal (
            "togglep",
            &dvar_toggle_print_f_hook,
            reinterpret_cast<cmd_function_s*> (0x141C98EE8));
          Cmd_AddCommandInternal (
            "set",
            &dvar_set_f_hook,
            reinterpret_cast<cmd_function_s*> (0x141C98F00));
          Cmd_AddCommandInternal (
            "seta",
            &dvar_seta_f_hook,
            reinterpret_cast<cmd_function_s*> (0x141C98F18));
          Cmd_AddCommandInternal (
            "reset",
            &dvar_reset_f_hook,
            reinterpret_cast<cmd_function_s*> (0x141C98F30));
          Cmd_AddCommandInternal (
            "setfromdvar",
            &dvar_set_from_dvar_f_hook,
            reinterpret_cast<cmd_function_s*> (0x141C98F48));
        }

        void
        dvar_init_hook ()
        {
          log::info << "dvar: init";

          *reinterpret_cast<unsigned char*> (dvar_initialized_flag) = 1;

          void* tls (engine_tls ());
          log::debug << "dvar: tls=" << tls;

          if (tls != nullptr)
          {
            int& allowed_flags (*reinterpret_cast<int*> (
              static_cast<unsigned char*> (tls) + tls_allowed_offset));

            // We must allow all flag categories during initialization to avoid
            // dvars registered prior to full context setup from being blocked
            // by our hook.
            //
            allowed_flags = -1;
          }

          dvar_add_commands_hook ();
          init_dvars ();
          register_dvars ();
          resolve_dvars ();
          apply_dvar_descriptions ();
        }

        void
        dvar_set_domain_func_hook (dvar_t* dvar,
                                   bool (*callback) (dvar_t*, DvarValue*))
        {
          if (dvar == nullptr)
            return;

          dvar->domainFunc = callback;

          if (callback == nullptr)
            return;

          const void* cb (reinterpret_cast<const void*> (callback));

          log::debug << "dvar: set_domain_func '" << log_safe (dvar->name)
                     << "' callback=" << cb;

          if (!executable_page (cb))
          {
            log::warning << "dvar: non-executable initial domain callback for '"
                         << log_safe (dvar->name) << "' callback=" << cb;
            return;
          }

          // We apply the domain function to verify the current active value.
          //
          DvarValue value (dvar->current);

          if (!callback (dvar, &value))
          {
            DvarValue reset_val (dvar->reset);
            set_variant (dvar, reset_val, DVAR_SOURCE_INTERNAL);
          }
        }

        void
        dvar_set_variant_hook (dvar_t* dvar,
                               DvarValue value,
                               DvarSetSource source)
        {
          log::debug << "dvar: set_variant '"
                     << (dvar != nullptr && dvar->name != nullptr ? dvar->name
                                                                  : "<null>")
                     << "' source=" << static_cast<int> (source);

          set_variant (dvar, value, source);
        }
      }
    }

    namespace
    {
      // IW4x dvar declaration table.
      //
      constexpr dvar_decl iw4x_dvar_declarations[] = {
        // Example declaration setup:
        //
        //  {
        //    "iw4x_cl_motd_enabled",
        //    DVAR_TYPE_BOOL,
        //    { .enabled = true },
        //    {},                          // domain: unconstrained for bool
        //    DVAR_ARCHIVE,
        //    "Whether IW4x MOTD is shown on connect."
        //  },
      };

      constexpr size_t iw4x_dvar_count =
        sizeof (iw4x_dvar_declarations) / sizeof (iw4x_dvar_declarations[0]);

      // Handles tailored for IW4x-owned dvars.
      //
      // These are populated by register_dvars() mirroring the
      // declaration-table order. (Always remember to add matching entries here
      // when adding to iw4x_dvar_declarations.)
      //
      // (None defined just yet.)

      // Dvar reference table.
      //
      // Each entry targets a dvar that IW4x code depends on. Keep in mind that
      // for required entries (where optional == false), encountering a missing
      // or mistyped dvar will throw a fatal error.
      //
      struct engine_dvar_ref
      {
        const char*  name;
        dvarType     expected_type;
        bool         optional;
        dvar_handle* out_handle;
        const char*  description;
      };

      constexpr engine_dvar_ref engine_dvar_refs[] = {
        // (none yet)
        //
      };

      constexpr size_t engine_dvar_ref_count =
        sizeof (engine_dvar_refs) / sizeof (engine_dvar_refs[0]);

      // Description patch table.
      //
      struct description_patch
      {
        const char* dvar_name;
        const char* description;
      };

      constexpr description_patch description_patches[] = {
        // (none yet)
      };

      constexpr size_t description_patch_count =
        sizeof (description_patches) / sizeof (description_patches[0]);

      // State guard isolating the initialization lifecycle.
      //
      // We track which lifecycle phases have concluded in order to flag
      // incorrect ordering double-initialization bugs before they cause havoc.
      //
      bool subsystem_initialized (false);
      bool dvars_registered (false);
      bool dvars_resolved (false);

      // Deep declaration validation payload.
      //
      // Always invoked by register_dvars() prior to executing any registration.
      // What we want here is to validates every entry in iw4x_dvar_declarations
      // and terminates via Com_Error if an invalid entry is found.
      //
      void
      validate_dvar_declarations ()
      {
        for (size_t i (0); i < iw4x_dvar_count; ++i)
        {
          const dvar_decl& decl (iw4x_dvar_declarations[i]);

          if (decl.name == nullptr || decl.name[0] == '\0')
            Com_Error (ERR_FATAL,
                       "dvar: declaration %zu has null or empty name",
                       i);

          if (decl.description == nullptr)
            Com_Error (ERR_FATAL,
                       "dvar: declaration '%s' has null description",
                       decl.name);

          if ((static_cast<int> (decl.flags) &
               static_cast<int> (DVAR_EXTERNAL)) != 0)
            Com_Error (ERR_FATAL,
                       "dvar: declaration '%s' sets DVAR_EXTERNAL - "
                       "this flag is reserved for config-parser dvars",
                       decl.name);

          // Check the list for duplicate names.
          //
          for (size_t j (0); j < i; ++j)
          {
            if (iw4x_dvar_declarations[j].name != nullptr &&
                I_stricmp (decl.name, iw4x_dvar_declarations[j].name) == 0)
              Com_Error (ERR_FATAL,
                         "dvar: duplicate declaration name '%s' "
                         "at indices %zu and %zu",
                         decl.name, j, i);
          }

          // Check type-specific constraints.
          //
          switch (decl.type)
          {
            case DVAR_TYPE_ENUM:
              if (decl.domain.enumeration.stringCount <= 0)
                Com_Error (
                  ERR_FATAL,
                  "dvar: declaration '%s' (ENUM) has " "stringCount <= 0",
                  decl.name);

              if (decl.domain.enumeration.strings == nullptr)
                Com_Error (ERR_FATAL,
                           "dvar: declaration '%s' (ENUM) has null strings",
                           decl.name);

              if (decl.default_value.integer < 0 ||
                  decl.default_value.integer >=
                    decl.domain.enumeration.stringCount)
                Com_Error (ERR_FATAL,
                           "dvar: declaration '%s' (ENUM) default index %d "
                           "is outside [0, %d)",
                           decl.name,
                           decl.default_value.integer,
                           decl.domain.enumeration.stringCount);
              break;

            case DVAR_TYPE_STRING:
              if (decl.default_value.string == nullptr)
                Com_Error (ERR_FATAL,
                           "dvar: declaration '%s' (STRING) has null "
                           "default_value.string",
                           decl.name);
              break;

            case DVAR_TYPE_INT:
              if (decl.domain.integer.min > decl.domain.integer.max)
                Com_Error (
                  ERR_FATAL,
                  "dvar: declaration '%s' (INT) domain min %d > max %d",
                  decl.name,
                  decl.domain.integer.min,
                  decl.domain.integer.max);

              if (!value_in_domain (decl.type, decl.default_value, decl.domain))
                Com_Error (ERR_FATAL,
                           "dvar: declaration '%s' (INT) default value %d "
                           "is outside domain [%d, %d]",
                           decl.name,
                           decl.default_value.integer,
                           decl.domain.integer.min,
                           decl.domain.integer.max);
              break;

            case DVAR_TYPE_INT64:
              if (!value_in_domain (decl.type, decl.default_value, decl.domain))
                Com_Error (
                  ERR_FATAL,
                  "dvar: declaration '%s' (INT64) default value " "%" PRId64
                  " is outside domain [%" PRId64 ", %" PRId64 "]",
                  decl.name,
                  decl.default_value.integer64,
                  decl.domain.integer64.min,
                  decl.domain.integer64.max);
              break;

            case DVAR_TYPE_UINT64:
              if (!value_in_domain (decl.type, decl.default_value, decl.domain))
                Com_Error (
                  ERR_FATAL,
                  "dvar: declaration '%s' (UINT64) default value " "%" PRIu64
                  " is outside domain [%" PRIu64 ", %" PRIu64 "]",
                  decl.name,
                  decl.default_value.unsignedInt64,
                  decl.domain.unsignedInt64.min,
                  decl.domain.unsignedInt64.max);
              break;

            case DVAR_TYPE_FLOAT:
              if (!value_in_domain (decl.type, decl.default_value, decl.domain))
                Com_Error (ERR_FATAL,
                           "dvar: declaration '%s' (FLOAT) default value %g "
                           "is outside domain [%g, %g]",
                           decl.name,
                           static_cast<double> (decl.default_value.value),
                           static_cast<double> (decl.domain.value.min),
                           static_cast<double> (decl.domain.value.max));
              break;

            default:
              // For BOOL, COLOR, and vector types, domain validation logic is
              // either true (as with BOOL and COLOR) or delegated downstream to
              // value_in_domain which covers FLOAT_2/3/4 scenarios.
              //
              if (!value_in_domain (decl.type, decl.default_value, decl.domain))
                Com_Error (ERR_FATAL,
                           "dvar: declaration '%s' (type %d) default value "
                           "fails domain check",
                           decl.name, static_cast<int> (decl.type));
              break;
          }
        }

        log::debug << "dvar: validated " << iw4x_dvar_count
                   << " declaration(s)";
      }
    }

    void
    init_dvars ()
    {
      if (subsystem_initialized)
      {
        log::warning << "dvar: init_dvars() called more than once - ignored";
        return;
      }

      subsystem_initialized = true;
    }

    void
    register_dvars ()
    {
      if (!subsystem_initialized)
        Com_Error (ERR_FATAL,
                   "dvar: register_dvars() called before init_dvars()");

      if (dvars_registered)
        Com_Error (ERR_FATAL, "dvar: register_dvars() called more than once");

      log::info << "dvar: registering " << iw4x_dvar_count << " IW4x dvar(s)";

      validate_dvar_declarations ();

      // Register each declared dvar in our predefined table order. Notice that
      // the external handles array aligns parallel to iw4x_dvar_declarations.
      // (Whenever future handles are actually appended, they will naturally be
      // assigned right here.)
      //
      for (size_t i (0); i < iw4x_dvar_count; ++i)
      {
        const dvar_decl& decl (iw4x_dvar_declarations[i]);

        dvar_t* ptr (register_variant (decl.name,
                                       decl.type,
                                       decl.flags,
                                       decl.default_value,
                                       decl.domain,
                                       decl.description));

        if (ptr == nullptr)
          Com_Error (ERR_FATAL,
                     "dvar: register_variant returned null for '%s'",
                     decl.name);

        log::info << "dvar: registered IW4x dvar '" << decl.name << "'";

        // Direct assignment targeting the corresponding exported handle. (Since
        // none exist yet, simply ignore for now. But add assignments later when
        // the handle array physically grows.
        //
        (void) i;
        (void) ptr;
      }

      dvars_registered = true;
    }

    void
    resolve_dvars ()
    {
      if (!dvars_registered)
        Com_Error (ERR_FATAL,
                   "dvar: resolve_dvars() called before register_dvars()");

      if (dvars_resolved)
      {
        log::warning << "dvar: resolve_dvars() called more than once - ignored";
        return;
      }

      log::info << "dvar: resolving " << engine_dvar_ref_count
                << " engine dvar reference(s)";

      for (size_t i (0); i < engine_dvar_ref_count; ++i)
      {
        const engine_dvar_ref& ref (engine_dvar_refs[i]);

        assert (ref.name != nullptr);
        assert (ref.out_handle != nullptr);

        dvar_t* ptr (find_dvar_raw (ref.name));

        // When a required dvar undeniably turns up absent, we emit a terminal
        // diagnostic and bail out.
        //
        if (ptr == nullptr && !ref.optional)
          Com_Error (ERR_FATAL,
                     "dvar: required engine dvar '%s' (type %d) was not "
                     "found during resolve_dvars() - the engine may not "
                     "have registered it yet, or the name is incorrect",
                     ref.name,
                     static_cast<int> (ref.expected_type));

        // If we encounter a type mismatch: log and either terminate (if
        // required) or emit an ignorable warning (if marked optional).
        //
        if (ptr != nullptr && ref.expected_type != DVAR_TYPE_COUNT &&
            ptr->type != ref.expected_type)
        {
          if (!ref.optional)
            Com_Error (ERR_FATAL,
                       "dvar: engine dvar '%s' has type %d but type %d was "
                       "required - ABI mismatch or wrong dvar name",
                       ref.name,
                       static_cast<int> (ptr->type),
                       static_cast<int> (ref.expected_type));
          else
          {
            log::warning << "dvar: optional engine dvar '" << ref.name
                         << "' has type " << static_cast<int> (ptr->type)
                         << " (expected "
                         << static_cast<int> (ref.expected_type)
                         << ") - handle left invalid";
            ptr = nullptr;
          }
        }

        *ref.out_handle = {ptr, ref.expected_type};

        log::info << "dvar: resolved '" << ref.name << "' → "
                  << (ptr != nullptr ? "ok" : "optional-miss");
      }

      dvars_resolved = true;
      log::info << "dvar: resolution complete";
    }

    void
    apply_dvar_descriptions ()
    {
      log::info << "dvar: applying " << description_patch_count
                << " description patch(es)";

      for (size_t i (0); i < description_patch_count; ++i)
      {
        const description_patch& patch (description_patches[i]);

        assert (patch.dvar_name != nullptr);
        assert (patch.description != nullptr);

        dvar_t* dvar (find_dvar_raw (patch.dvar_name));

        if (dvar == nullptr)
        {
          log::warning << "dvar: description patch target '" << patch.dvar_name
                       << "' not found - patch skipped";
          continue;
        }

        set_description (dvar, patch.description);

        log::debug << "dvar: patched description for '" << patch.dvar_name
                   << "'";
      }

      // Conclude by pushing any valid descriptions from the mapped
      // engine_dvar_refs entries, given they feature non-null description
      // fields and their corresponding handles were resolved beforehand.
      //
      for (size_t i (0); i < engine_dvar_ref_count; ++i)
      {
        const engine_dvar_ref& ref (engine_dvar_refs[i]);

        if (ref.description == nullptr)
          continue;

        if (ref.out_handle == nullptr || ref.out_handle->ptr == nullptr)
          continue;

        set_description (ref.out_handle->ptr, ref.description);

        log::debug << "dvar: applied resolved description for '" << ref.name
                   << "'";
      }
    }

    bool
    dvar_handle_valid (dvar_handle h)
    {
      if (h.ptr == nullptr)
        return false;

      if (h.expected_type != DVAR_TYPE_COUNT && h.ptr->type != h.expected_type)
      {
        log::warning << "dvar: handle type mismatch for '"
                     << log_safe (h.ptr->name) << "': expected type "
                     << static_cast<int> (h.expected_type)
                     << " but dvar has type " << static_cast<int> (h.ptr->type);
        return false;
      }

      return true;
    }

    dvar_handle
    require_engine_dvar (const char* name, dvarType expected_type)
    {
      assert (name != nullptr);

      dvar_t* ptr (find_dvar_raw (name));

      if (ptr == nullptr)
        Com_Error (ERR_FATAL,
                   "dvar: require_engine_dvar('%s', type=%d) failed: "
                   "dvar not found - it may not be registered yet, or "
                   "the name is misspelled",
                   name,
                   static_cast<int> (expected_type));

      if (expected_type != DVAR_TYPE_COUNT && ptr->type != expected_type)
        Com_Error (ERR_FATAL,
                   "dvar: require_engine_dvar('%s') type mismatch: "
                   "expected type %d but dvar has type %d - "
                   "ABI mismatch or wrong dvar name",
                   name,
                   static_cast<int> (expected_type),
                   static_cast<int> (ptr->type));

      return {ptr, expected_type};
    }

    dvar_handle
    find_engine_dvar (const char* name, dvarType expected_type)
    {
      if (name == nullptr || name[0] == '\0')
      {
        log::warning << "dvar: find_engine_dvar() called with null/empty name";
        return {nullptr, expected_type};
      }

      dvar_t* ptr (find_dvar_raw (name));

      if (ptr == nullptr)
        return {nullptr, expected_type};

      if (expected_type != DVAR_TYPE_COUNT && ptr->type != expected_type)
      {
        log::warning << "dvar: find_engine_dvar('" << name
                     << "') type mismatch: "
                     << "expected type " << static_cast<int> (expected_type)
                     << " but dvar has type " << static_cast<int> (ptr->type)
                     << " - handle returned invalid";
        return {nullptr, expected_type};
      }

      return {ptr, expected_type};
    }

    namespace
    {
      void
      log_invalid_handle (const char* operation,
                          dvar_handle h,
                          const char* expected_type_name)
      {
        if (h.ptr == nullptr)
        {
          log::warning << "dvar: " << operation
                       << ": called with invalid (null) handle; "
                       << "expected " << expected_type_name << " dvar";
        }
        else
        {
          log::warning << "dvar: " << operation << ": type mismatch on '"
                       << log_safe (h.ptr->name) << "': expected "
                       << expected_type_name << " (type "
                       << static_cast<int> (h.expected_type) << ")"
                       << " but dvar has type "
                       << static_cast<int> (h.ptr->type);
        }
      }
    }

    bool
    get_dvar_bool (dvar_handle h)
    {
      if (!dvar_handle_valid (h))
      {
        log_invalid_handle ("get_dvar_bool", h, "BOOL");
        return false;
      }

      return h.ptr->current.enabled;
    }

    int
    get_dvar_int (dvar_handle h)
    {
      if (!dvar_handle_valid (h))
      {
        log_invalid_handle ("get_dvar_int", h, "INT or ENUM");
        return 0;
      }

      if (h.ptr->type == DVAR_TYPE_ENUM)
        return h.ptr->current.integer;

      return h.ptr->current.integer;
    }

    float
    get_dvar_float (dvar_handle h)
    {
      if (!dvar_handle_valid (h))
      {
        log_invalid_handle ("get_dvar_float", h, "FLOAT");
        return 0.0f;
      }

      return h.ptr->current.value;
    }

    const char*
    get_dvar_string (dvar_handle h)
    {
      if (!dvar_handle_valid (h))
      {
        log_invalid_handle ("get_dvar_string", h, "STRING");
        return "";
      }

      return h.ptr->current.string != nullptr ? h.ptr->current.string : "";
    }

    int64_t
    get_dvar_int64 (dvar_handle h)
    {
      if (!dvar_handle_valid (h))
      {
        log_invalid_handle ("get_dvar_int64", h, "INT64");
        return 0;
      }

      return h.ptr->current.integer64;
    }

    uint64_t
    get_dvar_uint64 (dvar_handle h)
    {
      if (!dvar_handle_valid (h))
      {
        log_invalid_handle ("get_dvar_uint64", h, "UINT64");
        return 0;
      }

      return h.ptr->current.unsignedInt64;
    }

    void
    set_dvar_bool (dvar_handle h, bool value, DvarSetSource src)
    {
      if (!dvar_handle_valid (h))
      {
        log_invalid_handle ("set_dvar_bool", h, "BOOL");
        return;
      }

      DvarValue v {};
      v.enabled = value;
      set_variant (h.ptr, v, src);
    }

    void
    set_dvar_int (dvar_handle h, int value, DvarSetSource src)
    {
      if (!dvar_handle_valid (h))
      {
        log_invalid_handle ("set_dvar_int", h, "INT or ENUM");
        return;
      }

      DvarValue v {};
      v.integer = value;
      set_variant (h.ptr, v, src);
    }

    void
    set_dvar_float (dvar_handle h, float value, DvarSetSource src)
    {
      if (!dvar_handle_valid (h))
      {
        log_invalid_handle ("set_dvar_float", h, "FLOAT");
        return;
      }

      DvarValue v {};
      v.value = value;
      set_variant (h.ptr, v, src);
    }

    void
    set_dvar_string (dvar_handle h, const char* value, DvarSetSource src)
    {
      if (!dvar_handle_valid (h))
      {
        log_invalid_handle ("set_dvar_string", h, "STRING");
        return;
      }

      DvarValue v {};
      v.string = value != nullptr ? value : "";
      set_variant (h.ptr, v, src);
    }

    void
    reset_dvar (dvar_handle h, DvarSetSource src)
    {
      if (!dvar_handle_valid (h))
      {
        log_invalid_handle ("reset_dvar", h, "any");
        return;
      }

      DvarValue v (h.ptr->reset);
      set_variant (h.ptr, v, src);
    }

    dvar_module::
    dvar_module ()
    {
      detour (Dvar_AddCommands, &dvar_add_commands_hook);
      detour (Dvar_Command, &dvar_command_hook);
      detour (Dvar_GetCombinedString, &dvar_get_combined_string_hook);
      detour (Dvar_Reset_f, &dvar_reset_f_hook);
      detour (Dvar_SetA_f, &dvar_seta_f_hook);
      detour (Dvar_SetFromDvar_f, &dvar_set_from_dvar_f_hook);
      detour (Dvar_Set_f, &dvar_set_f_hook);
      detour (Dvar_ToggleInternal, &dvar_toggle_internal_hook);
      detour (Dvar_TogglePrint_f, &dvar_toggle_print_f_hook);
      detour (Dvar_Init, &dvar_init_hook);
      detour (Dvar_ResetScriptInfo, &dvar_reset_script_info_hook);
      detour (Dvar_Reset, &dvar_reset_hook);
      detour (Dvar_AddFlags, &dvar_add_flags_hook);
      detour (Dvar_UpdateResetValue, &dvar_update_reset_value_hook);
      detour (Dvar_FindMalleableVar, &dvar_find_var_hook);
      detour (Dvar_ForEach, &dvar_for_each_hook);
      detour (Dvar_RegisterVariant, &dvar_register_variant_hook);
      detour (Dvar_RegisterInt, &dvar_register_int_hook);
      detour (Dvar_RegisterBool, &dvar_register_bool_hook);
      detour (Dvar_RegisterFloat, &dvar_register_float_hook);
      detour (Dvar_RegisterString, &dvar_register_string_hook);
      detour (Dvar_RegisterEnum, &dvar_register_enum_hook);
      detour (Dvar_RegisterColor, &dvar_register_color_hook);
      detour (Dvar_RegisterVec2, &dvar_register_vec2_hook);
      detour (Dvar_RegisterVec3, &dvar_register_vec3_hook);
      detour (Dvar_RegisterVec3Color, &dvar_register_vec3_color_hook);
      detour (Dvar_RegisterVec4, &dvar_register_vec4_hook);
      detour (Dvar_ValueInDomain, &dvar_value_in_domain_hook);
      detour (Dvar_ValuesEqual, &dvar_values_equal_hook);
      detour (Dvar_SetLatchedValue, &dvar_set_latched_value_hook);
      detour (Dvar_StringToEnum, &dvar_string_to_enum_hook);
      detour (Dvar_StringToValue, &dvar_string_to_value_hook);
      detour (Dvar_StringToColor, &dvar_string_to_color_hook);
      detour (Dvar_GetBool, &dvar_get_bool_hook);
      detour (Dvar_GetFloat, &dvar_get_float_hook);
      detour (Dvar_GetIntByName, &dvar_get_int_by_name_hook);
      detour (Dvar_GetString, &dvar_get_string_hook);
      detour (Dvar_GetVariantString, &dvar_get_variant_string_hook);
      detour (Dvar_SetBool, &dvar_set_bool_hook);
      detour (Dvar_SetBoolByName, &dvar_set_bool_by_name_hook);
      detour (Dvar_SetFloat, &dvar_set_float_hook);
      detour (Dvar_SetInt, &dvar_set_int_hook);
      detour (Dvar_SetIntByName, &dvar_set_int_by_name_hook);
      detour (Dvar_SetString, &dvar_set_string_hook);
      detour (Dvar_SetStringByName, &dvar_set_string_by_name_hook);
      detour (Dvar_SetFromStringByName, &dvar_set_from_string_by_name_hook);
      detour (Dvar_SetFromStringFromSource, &dvar_set_from_string_from_source_hook);
      detour (Dvar_SetCommand, &dvar_set_command_hook);
      detour (Dvar_ValueToString, &dvar_value_to_string_hook);
      detour (Dvar_DisplayableValue, &dvar_displayable_value_hook);
      detour (Dvar_SetDomainFunc, &dvar_set_domain_func_hook);
      detour (Dvar_SetVariant, &dvar_set_variant_hook);
    }
  }
}
