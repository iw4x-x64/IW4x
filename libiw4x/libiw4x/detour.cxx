#include <libiw4x/detour.hxx>

#include <Zydis/Zydis.h>

#include <cstdint>
#include <cstring>
#include <stdexcept>

#include <libiw4x/utility-win32.hxx>

using namespace std;

namespace iw4x
{
  constexpr int32_t min_hook_size (14);
  constexpr int32_t max_hook_size (32);
  constexpr int64_t max_hook_disp (INT32_MAX);
  constexpr size_t  max_instruction_expansion (8);

  struct prologue_data
  {
    size_t ds;
    size_t di;
    ZydisDecodedInstruction ins[max_hook_size];
    ZydisDecodedOperand ops[max_hook_size][ZYDIS_MAX_OPERAND_COUNT];

    // Zero out the tracking sizes. The underlying structural arrays will be
    // overwritten as we decode the instructions, so there is no need to waste
    // CPU cycles clearing them here.
    //
    prologue_data ()
      : ds (0), di (0) {}
  };

  struct frame_data
  {
    prologue_data p;
    void* f;
    size_t cs;

    frame_data (prologue_data const& p_, void* f_, size_t cs_)
      : p (p_), f (f_), cs (cs_) {}
  };

  struct relocation_data
  {
    void* f;
    size_t ds;
    size_t rd;

    relocation_data (void* f_, size_t ds_, size_t rd_)
      : f (f_), ds (ds_), rd (rd_) {}
  };

  void
  detour (void*& t, void* s)
  {
    auto
    decode ([] (void* t) -> prologue_data
    {
      ZydisDecoder d;
      ZydisDecoderInit (&d, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);

      prologue_data p;

      // Keep decoding instructions until we have accumulated at least 14 bytes,
      // which is the exact minimum size required to place our absolute jump.
      // Note that since we cannot split x86 instructions halfway through, we
      // will likely end up pulling in a little bit more than the strict
      // minimum.
      //
      while (p.ds < min_hook_size && p.di < max_hook_size)
      {
        if (ZYAN_FAILED (
              ZydisDecoderDecodeFull (&d,
                                      static_cast<uint8_t*> (t) + p.ds,
                                      ZYDIS_MAX_INSTRUCTION_LENGTH,
                                      &p.ins[p.di],
                                      p.ops[p.di])))
          throw runtime_error ("unable to decode instruction");

        p.ds += p.ins[p.di].length;
        p.di++;
      }

      return p;
    });

    auto
    allocate ([] (void* t, prologue_data& p) -> frame_data
    {
      SYSTEM_INFO si;
      GetSystemInfo (&si);

      size_t ag (si.dwAllocationGranularity);
      size_t as (p.ds + (p.di * max_instruction_expansion) + min_hook_size);

      auto ta (reinterpret_cast<uint64_t> (t));
      uintptr_t min_a (ta > max_hook_disp ? ta - max_hook_disp : 0);
      uintptr_t max_a (ta < UINTPTR_MAX - max_hook_disp ? ta + max_hook_disp
                                                        : UINTPTR_MAX);

      // Find an isolated memory frame that is located close to the target
      // function. The requirement here is to stay within the 2GB boundary so
      // that any 32-bit RIP-relative addressing used in the stolen prologue
      // instructions remains valid after we relocate them. We do this by
      // walking the page tables both up and down.
      //
      auto
      probe ([as, ag, min_a, max_a] (uintptr_t addr, bool down) -> void*
      {
        MEMORY_BASIC_INFORMATION mi;

        while (addr >= min_a && addr <= max_a &&
               VirtualQuery (reinterpret_cast<void*> (addr), &mi, sizeof (mi)))
        {
          uintptr_t base (reinterpret_cast<uintptr_t> (mi.BaseAddress));
          uintptr_t size (mi.RegionSize);

          if (mi.State == MEM_FREE && size >= as)
          {
            uintptr_t a (down ? (base + size - as) & ~(ag - 1)
                              : (base + ag - 1) & ~(ag - 1));

            if (a >= base && a + as <= base + size && a >= min_a && a <= max_a)
            {
              if (void* m = VirtualAlloc (reinterpret_cast<void*> (a),
                                          as,
                                          MEM_COMMIT | MEM_RESERVE,
                                          PAGE_EXECUTE_READWRITE))
                return m;
            }
          }

          if (down)
          {
            if (base == 0) break;
            addr = base - 1;
          }
          else
          {
            addr = base + size;
            if (addr < base) break;
          }
        }

        return nullptr;
      });

      if (void* f = probe (ta, true))
        return frame_data (p, f, as);

      if (void* f = probe (ta, false))
        return frame_data (p, f, as);

      throw runtime_error ("unable to allocate isolated address frame");
    });

    auto
    relocate ([] (void* t, frame_data& f) -> relocation_data
    {
      auto fo (reinterpret_cast<uint8_t*> (f.f));
      auto fa (reinterpret_cast<uint64_t> (f.f));
      auto ta (reinterpret_cast<uint64_t> (t));

      size_t rd (0);
      uint64_t ra (ta);

      auto byte_relative_only ([] (ZydisMnemonic mnemonic) -> bool
      {
        switch (mnemonic)
        {
        case ZYDIS_MNEMONIC_JCXZ:
        case ZYDIS_MNEMONIC_JECXZ:
        case ZYDIS_MNEMONIC_JRCXZ:
        case ZYDIS_MNEMONIC_LOOP:
        case ZYDIS_MNEMONIC_LOOPE:
        case ZYDIS_MNEMONIC_LOOPNE:
          return true;

        default:
          return false;
        }
      });

      // Re-encode the stolen prologue instructions and place them into our
      // newly allocated trampoline frame. Because the physical distance to the
      // original target has changed, any RIP-relative memory accesses or
      // relative branches must be adjusted to point exactly to their original
      // destinations.
      //
      for (size_t i (0); i < f.p.di; ++i)
      {
        ZydisEncoderRequest r;
        memset (&r, 0, sizeof (r));

        ZydisDecodedInstruction* ri (&f.p.ins[i]);
        ZydisDecodedOperand* ro (f.p.ops[i]);
        ZyanU8 rv (ri->operand_count_visible);

        if (ZYAN_FAILED (
              ZydisEncoderDecodedInstructionToEncoderRequest (ri, ro, rv, &r)))
          throw runtime_error ("unable to create encoder request");

        for (ZyanU8 n (0); n < rv; ++n)
        {
          if (ro[n].type == ZYDIS_OPERAND_TYPE_MEMORY &&
              ro[n].mem.base == ZYDIS_REGISTER_RIP)
          {
            int64_t dv (ro[n].mem.disp.value);
            int64_t at (static_cast<int64_t> (ra + ri->length + dv));

            r.operands[n].mem.displacement = at;
          }
          else if (ro[n].type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
                   ro[n].imm.is_relative &&
                   ri->mnemonic != ZYDIS_MNEMONIC_XBEGIN)
          {
            int64_t dv (ro[n].imm.is_signed
                        ? ro[n].imm.value.s
                        : static_cast<int64_t> (ro[n].imm.value.u));
            int64_t at (static_cast<int64_t> (ra + ri->length + dv));

            // We must catch cases where a relative branch attempts to jump
            // internally within the relocated prologue block itself. If it
            // does, we cannot simply re-encode it because the relative layout
            // has shifted under us.
            //
            if (at >= static_cast<int64_t> (ta) &&
                at < static_cast<int64_t> (ta + f.p.ds))
              throw runtime_error ("relative branch into relocated prologue");

            r.operands[n].imm.s = at;
            r.operands[n].imm.u = static_cast<ZyanU64> (at);

            if (r.branch_width == ZYDIS_BRANCH_WIDTH_8 &&
                !byte_relative_only (ri->mnemonic))
            {
              r.branch_type = ZYDIS_BRANCH_TYPE_NEAR;
              r.branch_width = ZYDIS_BRANCH_WIDTH_32;
            }
          }
        }

        ZyanUSize rl (f.cs - rd);

        if (ZYAN_FAILED (ZydisEncoderEncodeInstructionAbsolute (&r,
                                                                fo + rd,
                                                                &rl,
                                                                fa + rd)))
          throw runtime_error ("unable to encode relocated instruction");

        rd += rl;
        ra += ri->length;
      }

      return relocation_data (f.f, f.p.ds, rd);
    });

    auto
    apply ([] (void*& t, void* s, relocation_data& r) -> void
    {
      auto
      commit ([] (uint8_t* b, void* dst)
      {
        ZydisEncoderRequest req;
        memset (&req, 0, sizeof (req));

        req.mnemonic = ZYDIS_MNEMONIC_JMP;
        req.machine_mode = ZYDIS_MACHINE_MODE_LONG_64;
        req.operand_count = 1;

        ZydisEncoderOperand* o (&req.operands[0]);
        o->type = ZYDIS_OPERAND_TYPE_MEMORY;
        o->mem.base = ZYDIS_REGISTER_RIP;
        o->mem.displacement = 0;
        o->mem.size = sizeof (ZyanU64);

        ZyanU8 in[ZYDIS_MAX_INSTRUCTION_LENGTH];
        ZyanUSize il (sizeof (in));

        if (ZYAN_FAILED (ZydisEncoderEncodeInstruction (&req, in, &il)))
          throw runtime_error ("unable to encode absolute jump");

        memcpy (b, in, il);
        memcpy (b + il, &dst, sizeof (dst));
      });

      auto to (reinterpret_cast<uint8_t*> (t));
      auto fo (reinterpret_cast<uint8_t*> (r.f));

      uint8_t* ta (fo + r.rd);
      void* re (reinterpret_cast<uint8_t*> (t) + r.ds);
      DWORD old_protect (0);

      // Write the absolute jump into the trampoline frame, pointing it back to
      // the remainder of the original function.
      //
      commit (ta, re);

      if (!VirtualProtect (to, r.ds, PAGE_EXECUTE_READWRITE, &old_protect))
        throw runtime_error ("unable to make detour target writable");

      // Write the actual hook jump over the original function prologue.
      //
      commit (to, s);

      size_t ps (14);
      if (r.ds > ps)
        memset (to + ps, 0x90, r.ds - ps);

      DWORD unused (0);
      VirtualProtect (to, r.ds, old_protect, &unused);
      VirtualProtect (fo, r.rd + min_hook_size, PAGE_EXECUTE_READ, &unused);

      FlushInstructionCache (GetCurrentProcess (), fo, r.rd + min_hook_size);
      FlushInstructionCache (GetCurrentProcess (), to, r.ds);

      t = fo;
    });

    prologue_data p (decode (t));
    frame_data f (allocate (t, p));
    relocation_data r (relocate (t, f));

    apply (t, s, r);
  }
}
