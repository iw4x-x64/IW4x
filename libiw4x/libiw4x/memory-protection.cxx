#include <libiw4x/memory-protection.hxx>

#include <cassert>
#include <cstdio>
#include <format>
#include <system_error>
#include <utility>

#include <libiw4x/utility-win32.hxx>

using namespace std;

namespace iw4x
{
  // Map a memory_access enumerator to the corresponding Windows base protection
  // constant. This does not include modifier bits as those are combined
  // separately.
  //
  static DWORD
  access_to_win32 (memory_access a) noexcept
  {
    switch (a)
    {
      case memory_access::none:          return PAGE_NOACCESS;
      case memory_access::read:          return PAGE_READONLY;
      case memory_access::read_write:    return PAGE_READWRITE;
      case memory_access::execute:       return PAGE_EXECUTE;
      case memory_access::execute_read:  return PAGE_EXECUTE_READ;
    }

    // Unreachable for a well-formed memory_access value. The switch is
    // exhaustive over the enum and this exists only to satisfy the compiler.
    //
    assert (false);
    return PAGE_NOACCESS;
  }

  // Map the protection_modifier flags to Windows modifier bits.
  //
  static DWORD
  modifiers_to_win32 (protection_modifier m) noexcept
  {
    DWORD f (0);

    if (has_modifier (m, protection_modifier::guard))
      f |= PAGE_GUARD;

    if (has_modifier (m, protection_modifier::no_cache))
      f |= PAGE_NOCACHE;

    if (has_modifier (m, protection_modifier::write_combine))
      f |= PAGE_WRITECOMBINE;

    return f;
  }

  // Produce the complete Windows protection DWORD for a page_protection value.
  //
  static DWORD
  to_win32 (page_protection p) noexcept
  {
    return access_to_win32 (p.access ()) | modifiers_to_win32 (p.modifiers ());
  }

  // Determine whether a raw Windows protection DWORD indicates executable
  // memory.
  //
  // Windows assigns the four PAGE_EXECUTE* constants to the values 0x10, 0x20,
  // 0x40, and 0x80 which is the high nibble of the low byte of DWORD. Testing
  // that nibble catches all four cases without enumerating them, and also
  // correctly handles any vendor-defined executable constants that might occupy
  // that nibble range.
  //
  static bool
  win32_flags_are_executable (uint32_t f) noexcept
  {
    return (f & 0xF0u) != 0;
  }

  // Determine whether a raw Windows protection DWORD requests write access.
  //
  // The write-capable constants are PAGE_READWRITE (0x04), PAGE_WRITECOPY
  // (0x08), PAGE_EXECUTE_READWRITE (0x40), and PAGE_EXECUTE_WRITECOPY (0x80).
  // Their combined mask is 0xCC. We test this mask against the low byte
  // (modifiers live above 0xFF and do not contribute write access themselves).
  //
  static bool
  win32_flags_request_write (uint32_t f) noexcept
  {
    return (f & 0xCCu) != 0;
  }

  // Region validation.
  //
  // Validate the memory region described by [base, base+size) against the
  // constraints required for a safe protection change.
  //
  static void
  validate_region (void* base,
                   size_t size,
                   uint32_t new_win32_flags,
                   protection_guard::private_policy policy)
  {
    MEMORY_BASIC_INFORMATION mi;

    if (!VirtualQuery (base, &mi, sizeof (mi)))
      throw system_error (GetLastError (),
                          system_category (),
                          format ("VirtualQuery failed for address {:#x}",
                                  reinterpret_cast<uintptr_t> (base)));

    // Committed check.
    //
    // MEM_RESERVE means address space has been claimed but no physical (or
    // pagefile) storage has been assigned. MEM_FREE means the address range
    // isn't ours at all. VirtualProtect on either type returns
    // ERROR_INVALID_ADDRESS. We surface a clearer message.
    //
    if (mi.State != MEM_COMMIT)
      throw runtime_error (
          format (
            "memory at {:#x} is not committed (VirtualQuery State: {:#x}); "
            "the range must be MEM_COMMIT before protection can be changed",
            reinterpret_cast<uintptr_t> (base),
            static_cast<unsigned> (mi.State)));

    // Cross-region check.
    //
    // VirtualQuery describes the longest contiguous run of pages beginning at
    // base that share the same State, Type, and AllocationProtect. If our
    // requested range exceeds that run, the remainder belongs to a different
    // region with potentially different attributes. A single VirtualProtect
    // call across that boundary would affect pages we never validated.
    //
    if (size > static_cast<size_t> (mi.RegionSize))
      throw runtime_error (
          format ("{} byte range at {:#x} spans multiple memory regions; "
                  "the first region covers only {} bytes",
                  size,
                  reinterpret_cast<uintptr_t> (base),
                  static_cast<size_t> (mi.RegionSize)));

    // Private memory check.
    //
    // MEM_PRIVATE is reported for VirtualAlloc-owned pages, heap blocks, and
    // thread stacks. The heap manager shares pages with its own bookkeeping
    // metadata and making a heap page read-only or inaccessible will corrupt it
    // on the next coalesce or header update. Since we cannot distinguish
    // VirtualAlloc-owned from heap-managed at the OS level, the default policy
    // is to reject all MEM_PRIVATE. The for_virtual_alloc_region() factory opts
    // in explicitly.
    //
    if (mi.Type == MEM_PRIVATE &&
        policy == protection_guard::private_policy::reject)
    {
      throw runtime_error (
          format ("memory at {:#x} is MEM_PRIVATE (could be heap or stack); "
                  "use protection_guard::for_virtual_alloc_region() "
                  "if this is a VirtualAlloc-owned allocation",
                  reinterpret_cast<uintptr_t> (base)));
    }

    // Mapped-view write capability check.
    //
    // For memory-mapped views (MEM_MAPPED), write permission is ultimately
    // governed by the access rights of the section handle that was used to
    // create the mapping. MapViewOfFile records that access grant in
    // AllocationProtect. If the section was mapped read-only, VirtualProtect
    // cannot grant write access and the physical backing store was never given
    // a write handle. We detect this before calling VirtualProtect so the
    // caller receives a logical error explaining the root cause rather than a
    // cryptic ERROR_ACCESS_DENIED.
    //
    if (mi.Type == MEM_MAPPED && win32_flags_request_write (new_win32_flags))
    {
      DWORD ap (mi.AllocationProtect & 0xFFu); // Base flags only, no modifiers.

      bool section_allows_write (ap == PAGE_READWRITE     ||
                                 ap == PAGE_WRITECOPY     ||
                                 ap == PAGE_EXECUTE_READWRITE ||
                                 ap == PAGE_EXECUTE_WRITECOPY);

      if (!section_allows_write)
        throw runtime_error (
            format ("requesting write access ({:#x}) on a mapped view whose "
                    "section handle was opened read-only "
                    "(AllocationProtect: {:#x}); "
                    "the mapping must be recreated with write access",
                    static_cast<unsigned> (new_win32_flags),
                    static_cast<unsigned> (mi.AllocationProtect)));
    }
  }

  size_t
  system_page_size () noexcept
  {
    // We initialise the static on first call through a lambda so that the query
    // is deferred until the first actual use rather than happening at program
    // start during static initialisation. The lambda executes exactly once
    // (guaranteed by the C++ standard for function-local statics), so there is
    // no race condition.
    //
    static const size_t ps ([]
    {
      SYSTEM_INFO si;
      GetSystemInfo (&si);
      return static_cast<size_t> (si.dwPageSize);
    } ());

    return ps;
  }

  page_address::
  page_address (void* a)
    : value_ (a)
  {
    if (a == nullptr)
      throw invalid_argument ("page_address: null is not a valid page address");

    auto n  (reinterpret_cast<uintptr_t> (a));
    auto ps (system_page_size ());

    if ((n & (ps - 1)) != 0)
    {
      // Compute the rounded-down base so we can offer it as a hint. The caller
      // likely has the right intent (protect a function, protect an allocation)
      // but is passing a pointer to something inside the page rather than the
      // page base itself.
      //
      auto aligned (n & ~(ps - 1));

      throw invalid_argument (
          format ("address {:#x} is not page-aligned (page size: {} bytes); "
                  "did you mean {:#x}?",
                  n, ps, aligned));
    }
  }

  // Round n up to the nearest multiple of the system page size.
  //
  static size_t
  round_up_to_page_size (size_t n) noexcept
  {
    auto ps (system_page_size ());
    return (n + ps - 1) & ~(ps - 1);
  }

  page_range::
  page_range (page_address b, size_t s)
      : base_ (b), size_ (round_up_to_page_size (s))
  {
    // We check for zero before the round-up so that we catch the caller's
    // intent. A zero-byte range is meaningless regardless of rounding.
    //
    if (s == 0)
      throw invalid_argument (
          "page_range: size must be greater than zero");
  }

  protection_modifier
  operator| (protection_modifier a, protection_modifier b) noexcept
  {
    return static_cast<protection_modifier> (
        static_cast<unsigned> (a) | static_cast<unsigned> (b));
  }

  bool
  has_modifier (protection_modifier flags, protection_modifier bit) noexcept
  {
    return (static_cast<unsigned> (flags) &
            static_cast<unsigned> (bit)) != 0;
  }

  page_protection::
  page_protection (memory_access a, protection_modifier m)
    : access_ (a), modifiers_ (m)
  {
    // PAGE_GUARD with PAGE_NOACCESS is an explicitly prohibited combination in
    // the Windows documentation. The guard mechanism works by first raising a
    // STATUS_GUARD_PAGE exception and then switching the page to its underlying
    // protection. If that underlying protection is NOACCESS, the behaviour on
    // subsequent accesses is undefined (the OS clears the guard bit but the
    // page remains inaccessible, producing inconsistent semantics).
    //
    if (has_modifier (m, protection_modifier::guard) &&
        a == memory_access::none)
    {
      throw logic_error (
        "page_protection: protection_modifier::guard cannot be combined with "
        "memory_access::none; the guard mechanism requires a valid "
        "underlying access mode (PAGE_GUARD | PAGE_NOACCESS is invalid per "
        "Windows documentation)");
    }
  }

  bool
  page_protection::is_executable () const noexcept
  {
    return access_ == memory_access::execute ||
           access_ == memory_access::execute_read;
  }

  bool
  page_protection::is_writable () const noexcept
  {
    return access_ == memory_access::read_write;
  }

  bool
  page_protection::is_accessible () const noexcept
  {
    return access_ != memory_access::none;
  }

  protection_guard::
  protection_guard (page_range      r,
                    page_protection p,
                    uint32_t        win32_flags,
                    private_policy  policy)
  {
    void* base (r.base ().get ());
    size_t sz  (r.size ());

    validate_region (base, sz, win32_flags, policy);

    // Apply the new protection. VirtualProtect writes the previous protection
    // DWORD into prev_flags and we capture it for restoration. The
    // lpflOldProtect parameter is documented as required. That is, passing
    // nullptr causes an access violation, not a graceful failure.
    //
    DWORD prev_flags (0);

    if (!VirtualProtect (base,
                         sz,
                         static_cast<DWORD> (win32_flags),
                         &prev_flags))
    {
      throw system_error (GetLastError (),
                          system_category (),
                          format ("VirtualProtect failed for {:#x}+{}",
                                  reinterpret_cast<uintptr_t> (base),
                                  sz));
    }

    state_.emplace (active_state {r, p, static_cast<uint32_t> (prev_flags)});
  }

  protection_guard::
  protection_guard (page_range r, page_protection p)
    : protection_guard (move (r), p,
                        static_cast<uint32_t> (to_win32 (p)),
                        private_policy::reject)
  {
  }

  protection_guard protection_guard::
  for_virtual_alloc_region (page_range r, page_protection p)
  {
    return protection_guard (move (r), p,
                             static_cast<uint32_t> (to_win32 (p)),
                             private_policy::allow);
  }

  protection_guard::
  protection_guard (page_range r,
                    unsafe_execute_read_write_t,
                    const char* justification)
    // We store execute_read as the applied protection because page_protection
    // has no execute_read_write enumerator (that is the entire point of the
    // type). applied_protection() documents that it returns an approximation
    // for guards constructed via this path.
    //
    : protection_guard (move (r),
                        page_protection (memory_access::execute_read),
                        static_cast<uint32_t> (PAGE_EXECUTE_READWRITE),
                        private_policy::allow)
  {
    // Emit the justification on stderr in debug builds. We want RWX to be
    // auditable: anyone running the tests or a debug build can see every place
    // in the codebase that applied PAGE_EXECUTE_READWRITE and why.
    //
#ifndef NDEBUG
    fprintf (stderr,
             "[protection_guard] RWX (PAGE_EXECUTE_READWRITE) applied — %s\n",
             justification != nullptr ? justification
                                      : "(no justification provided)");
#else
    static_cast<void> (justification);
#endif
  }

  protection_guard::
  ~protection_guard ()
  {
    if (!state_)
      return;

    void*  base (state_->range.base ().get ());
    size_t sz   (state_->range.size ());
    DWORD  prev (static_cast<DWORD> (state_->previous_flags));

    // Restore the original protection.
    //
    // We pass the exact DWORD we captured from VirtualProtect at construction
    // time. Routing it through page_protection would be lossy (page_protection
    // cannot represent every OS-returned DWORD, e.g., PAGE_EXECUTE_READWRITE
    // has no enumerator) and a lossy restore could silently change the
    // effective protection.
    //
    DWORD discard (0);

    if (!VirtualProtect (base, sz, prev, &discard))
      terminate ();

    // Flush the instruction cache if the restored protection is executable.
    //
    // "Executable" means the pages are about to be eligible for instruction
    // fetch. If we were used as a code-patching guard (temporarily applying
    // read_write to an otherwise execute_read region), the CPU's instruction
    // pipeline may still hold pre-patch instructions from before our change.
    // The flush forces coherence between the data cache (which has the new
    // bytes) and the instruction cache (which may not yet).
    //
    // Note also that we test the restored DWORD rather than the applied
    // protection so that the check is always accurate even for guards
    // constructed via the RWX escape hatch.
    //
    if (win32_flags_are_executable (prev))
    {
      if (!FlushInstructionCache (GetCurrentProcess (), base, sz))
        terminate ();
    }
  }

  protection_guard::
  protection_guard (protection_guard&& other) noexcept
    : state_ (move (other.state_))
  {
    // Moving an optional transfers the value and leaves the source empty.
    // other.is_active() is now false and its destructor is a no-op.
  }

  protection_guard& protection_guard::
  operator= (protection_guard&& other) noexcept
  {
    if (this != &other)
    {
      // If we currently hold an active state, we must fulfil our restore
      // obligation before adopting other's state. We do this by moving our own
      // state into a temporary guard that destructs at the end of this block,
      // triggering the restore. After that, *this has an empty state_ and is
      // ready to receive other's state.
      //
      { protection_guard dying (move (*this)); }

      state_ = move (other.state_);
    }

    return *this;
  }

  void protection_guard::
  release () noexcept
  {
    state_.reset ();
  }

  bool protection_guard::
  is_active () const noexcept
  {
    return state_.has_value ();
  }

  page_range protection_guard::
  range () const noexcept
  {
    assert (is_active ());
    return state_->range;
  }

  page_protection protection_guard::
  applied_protection () const noexcept
  {
    assert (is_active ());
    return state_->applied;
  }
}
