#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include <libiw4x/export.hxx>

namespace iw4x
{
  // Return the system memory page size, in bytes.
  //
  // On Windows, this is determined at boot time by the hardware (4 KiB on
  // x86/x64) and never changes for the lifetime of the process. We query it
  // once and cache the result, making repeated calls essentially free.
  //
  std::size_t
  system_page_size () noexcept;

  // A memory address that has been explicitly validated to lie on a page
  // boundary.
  //
  // Specifically, VirtualProtect() silently rounds misaligned addresses down to
  // the enclosing page base. This rounding changes permissions for the entire
  // page regardless of what the caller intended, meaning any data or code
  // residing earlier on the same page is also affected. So here we require page
  // alignment at construction time to be deliberate about which page we are
  // operating on.
  //
  // Note that construction throws std::invalid_argument if the address is null
  // or is not aligned to the system page size. The exception text includes the
  // correctly aligned page base as a diagnostic hint.
  //
  class page_address
  {
  public:
    explicit
    page_address (void* address);

    void*
    get () const noexcept
    {
      return value_;
    }

    std::uintptr_t
    value () const noexcept
    {
      return reinterpret_cast<std::uintptr_t> (value_);
    }

    bool
    operator == (const page_address&) const noexcept = default;

  private:
    void* value_;
  };

  // A non-empty, page-aligned contiguous range of memory.
  //
  // The base address must satisfy the alignment constraint of page_address.
  // The byte count must be greater than zero. Internally the count is rounded
  // up to the next page boundary: since VirtualProtect() always operates on
  // whole pages, we surface that rounding in the type rather than hiding it in
  // the OS call. The rounded size is what gets stored and what is passed to
  // VirtualProtect.
  //
  // Construction throws std::invalid_argument for a null or misaligned base,
  // or for a zero size.
  //
  class page_range
  {
  public:
    explicit
    page_range (page_address base, std::size_t size);

    page_address
    base () const noexcept
    {
      return base_;
    }

    std::size_t
    size () const noexcept
    {
      return size_;
    }

  private:
    page_address base_;
    std::size_t size_;
  };

  // Base memory access permission for a page range.
  //
  // The five values cover every meaningful W^X-safe combination. The sixth
  // combination, write plus execute (PAGE_EXECUTE_READWRITE, or "RWX"), is
  // intentionally absent. Writable executable memory is the canonical
  // exploitation primitive: if someone can write arbitrary bytes to an
  // executable page, they can immediately run them. The type system makes that
  // combination inexpressible here.
  //
  // If your use case genuinely requires RWX, use the protection_guard
  // constructor that takes an unsafe_execute_read_write_t tag (see below), but
  // be aware that this path is deliberately verbose and requires a documented
  // justification.
  //
  enum class memory_access
  {
    none,
    read,
    read_write,
    execute,
    execute_read,
  };

  // Optional modifier bits that augment a base memory_access.
  //
  // These correspond directly to the Windows PAGE_GUARD, PAGE_NOCACHE, and
  // PAGE_WRITECOMBINE flags. They are combinable using operator|. Validity
  // is checked during page_protection construction.
  //
  enum class protection_modifier : unsigned
  {
    none          = 0x00u,
    guard         = 0x01u,
    no_cache      = 0x02u,
    write_combine = 0x04u,
  };

  protection_modifier
  operator| (protection_modifier lhs, protection_modifier rhs) noexcept;

  // Test whether a specific modifier bit is set in a combined flag value.
  //
  bool
  has_modifier (protection_modifier flags, protection_modifier bit) noexcept;

  // A validated, complete memory protection specification.
  //
  // It combines a base memory_access with zero or more protection_modifier
  // bits. Construction validates the combination. For example, Windows
  // explicitly documents PAGE_GUARD combined with PAGE_NOACCESS as invalid (the
  // guard mechanism must have a valid underlying access mode to arm itself). We
  // reject it at construction time so the caller receives a clear
  // std::logic_error rather than silent undefined behavior on the first access.
  //
  class page_protection
  {
  public:
    explicit
    page_protection (memory_access access,
                     protection_modifier modifiers = protection_modifier::none);

    memory_access
    access () const noexcept
    {
      return access_;
    }

    protection_modifier
    modifiers () const noexcept
    {
      return modifiers_;
    }

    bool
    is_executable () const noexcept;

    bool
    is_writable () const noexcept;

    bool
    is_accessible () const noexcept;

    bool
    operator == (const page_protection&) const noexcept = default;

  private:
    memory_access access_;
    protection_modifier modifiers_;
  };

  // Tag type for the RWX escape hatch.
  //
  // To request PAGE_EXECUTE_READWRITE (write and execute simultaneously), pass
  // the unsafe_execute_read_write constant as the second argument of the
  // corresponding protection_guard constructor, along with a justification
  // string explaining why RWX is necessary:
  //
  //   protection_guard g (
  //     range,
  //     unsafe_execute_read_write,
  //     "self-patching trampoline: no W^X hardware on this target");
  //
  // The tag makes the intent unambiguous at the call site. The justification
  // is present so that a reviewer reading the diff can understand the reasoning
  // immediately without tracing through headers. In debug builds, the
  // justification is also printed to stderr, making RWX usage visible in CI.
  //
  struct unsafe_execute_read_write_t
  {
    explicit
    unsafe_execute_read_write_t () noexcept = default;
  };

  inline constexpr unsafe_execute_read_write_t unsafe_execute_read_write {};

  // RAII guard for a scoped memory protection change.
  //
  // Applies a VirtualProtect() call on construction and issues the exact
  // inverse call on destruction, restoring the original Windows protection
  // DWORD. The guard is move-only, meaning exactly one instance at a time owns
  // the restore obligation.
  //
  // Construction (strong exception safety).
  //
  // We validate the memory region with VirtualQuery(), then call
  // VirtualProtect(). If either fails, an exception is thrown and no protection
  // has been changed. Either we own the full change, or we own none of it.
  //
  // Destruction (no-throw, terminate on failure).
  //
  // We call VirtualProtect() with the original protection DWORD. If the call
  // fails, std::terminate() is called. There is no safe recovery strategy: an
  // unrestorable protection change means the address space is in an unknown
  // state. Attempting to continue could mean leaving code writable or data
  // inaccessible, both of which are security defects. Termination is the
  // correct policy here.
  //
  // After restoring to a protection that includes execute permission, we call
  // FlushInstructionCache() for the range. If any code was written to the
  // region while the guard held a writable protection, the processor
  // instruction cache may still hold stale pre-patch instructions. The flush is
  // for coherence before the next fetch from those addresses. If
  // FlushInstructionCache also fails, std::terminate() is called for the same
  // reason.
  //
  // Memory type policy.
  //
  // The default constructor accepts MEM_IMAGE (loaded PE sections) and
  // MEM_MAPPED (file-mapping views). It rejects MEM_PRIVATE.
  //
  // MEM_PRIVATE is the type reported for VirtualAlloc()-owned pages, heap
  // blocks, and thread stacks. The Windows heap manager stores its bookkeeping
  // metadata in the same pages as user allocations. If a heap page is changed
  // to PAGE_READONLY or PAGE_NOACCESS, the allocator will fault the next time
  // it tries to write those headers. This typically occurs on the next
  // HeapAlloc() or HeapFree() call, far from the protection change, making the
  // crash very hard to diagnose.
  //
  // Windows provides no API to distinguish "VirtualAlloc()-owned private
  // memory" from "heap-managed private memory". The only safe default for a
  // general utility is therefore to reject the whole MEM_PRIVATE category. If
  // you own the backing allocation (allocated directly with VirtualAlloc(), not
  // via the heap), call for_virtual_alloc_region() instead. That factory
  // permits MEM_PRIVATE because you have explicitly asserted ownership.
  //
  class protection_guard
  {
  public:
    // Construct a guard for image-backed or memory-mapped regions.
    //
    // Accepted memory types:
    //   MEM_IMAGE  - Pages belonging to a loaded PE image (e.g., a DLL .text
    //                section). The image loader gives us this type; it is
    //                never heap or stack memory.
    //   MEM_MAPPED - Pages belonging to a MapViewOfFile region.
    //
    // Rejected memory types:
    //   MEM_PRIVATE - Could be heap, stack, or VirtualAlloc. Use the
    //                 for_virtual_alloc_region() factory if you own the
    //                 allocation.
    //
    // Additional checks:
    //   - The region must be committed (MEM_COMMIT); reserved or free pages
    //     have no physical backing.
    //   - The range must lie entirely within a single VirtualQuery region.
    //     Crossing a region boundary risks altering pages we never validated.
    //   - For MEM_MAPPED, if write access is requested, the section handle must
    //     have been opened for write access (checked via AllocationProtect).
    //     VirtualProtect() cannot grant write access that the section handle
    //     does not possess. We diagnose this early with a clear error.
    //
    // Throws:
    //   std::logic_error       - PAGE_GUARD | PAGE_NOACCESS combination.
    //   std::invalid_argument  - Misaligned base or zero size (from page_range).
    //   std::runtime_error     - Uncommitted, private, cross-region, or
    //                            write-incapable mapped view.
    //   std::system_error      - VirtualQuery or VirtualProtect failure.
    //
    explicit
    protection_guard (page_range range, page_protection protection);

    // Named factory for VirtualAlloc()-owned private regions.
    //
    // Use this when you explicitly allocated the backing pages with
    // VirtualAlloc() and wish to change their protection. The canonical use
    // case is a JIT compiler that writes machine code into a PAGE_READWRITE
    // buffer and then wants to mark it PAGE_EXECUTE_READ:
    //
    //   void* buf (VirtualAlloc (nullptr, size, MEM_COMMIT | MEM_RESERVE,
    //                            PAGE_READWRITE)); // ... emit machine code
    //                            into buf ...
    //   {
    //     auto g (protection_guard::for_virtual_alloc_region (
    //              page_range (buf, size),
    //              page_protection (memory_access::execute_read)));
    //
    //     // g destructs: restores PAGE_READWRITE, flushes instruction cache.
    //   }
    //
    // Note that by calling this factory you assert that no heap or stack
    // bookkeeping metadata resides in the target pages. All other validation
    // still applies.
    //
    // Throws: Same as the general constructor, minus the MEM_PRIVATE rejection.
    //
    static protection_guard
    for_virtual_alloc_region (page_range range, page_protection protection);

    // RWX constructor.
    //
    // Use this only when write and execute simultaneously are genuinely
    // required.
    //
    // Applies PAGE_EXECUTE_READWRITE. Requires the unsafe_execute_read_write
    // tag (see above) to reach this overload. The justification parameter is
    // mandatory. In debug builds it is written to stderr so that RWX usage is
    // visible in test logs and CI output.
    //
    // All other validation still applies. MEM_PRIVATE is accepted because RWX
    // is almost always used with VirtualAlloc-backed JIT pages.
    //
    // Note that applied_protection() on a guard constructed this way returns
    // execute_read as an approximation. The actual applied flags are RWX.
    //
    explicit
    protection_guard (page_range range,
                      unsafe_execute_read_write_t,
                      const char* justification);

    ~protection_guard ();

    protection_guard (protection_guard&&) noexcept;
    protection_guard& operator= (protection_guard&&) noexcept;

    protection_guard (const protection_guard&) = delete;
    protection_guard& operator= (const protection_guard&) = delete;

    // Release the restore obligation without performing the restore.
    //
    // After release, is_active returns false and the destructor is a no-op. The
    // caller takes responsibility for restoring the original protection and
    // flushing the instruction cache if appropriate. Use this when the changed
    // protection must outlive the lexical scope of the guard (for example, when
    // transferring ownership to a thread or a persistent data structure).
    //
    void
    release () noexcept;

    bool
    is_active () const noexcept;

    // The following accessors require is_active() to be true.
    //
    page_range
    range () const noexcept;

    page_protection
    applied_protection () const noexcept; // Approximate for RWX.

  private:
    // We store the previous protection as the raw Windows DWORD (as uint32_t to
    // keep Windows types out of the header) rather than as page_protection. The
    // page_protection type cannot represent every value the OS might return
    // (PAGE_EXECUTE_READWRITE has no enumerator in memory_access, for example).
    //
    struct active_state
    {
      page_range      range;
      page_protection applied;
      std::uint32_t   previous_flags;
    };

    std::optional<active_state> state_;

    // Controls whether MEM_PRIVATE pages are accepted. The public constructor
    // uses reject and for_virtual_alloc_region() uses allow.
    //
  public:
    enum class private_policy { reject, allow };

  private:
    // Internal constructor shared by the public constructor, the named factory,
    // and the RWX escape hatch. Performs VirtualQuery() validation,
    // VirtualProtect() call, and state capture. The win32_flags parameter is
    // the actual DWORD to pass to VirtualProtect(); it may differ from
    // to_win32(protection) in the RWX case.
    //
    explicit
    protection_guard (page_range      range,
                      page_protection protection,
                      std::uint32_t   win32_flags,
                      private_policy  policy);
  };
}
