#include <libiw4x/detour.hxx>

#include <windows.h>
#include <Zydis/Zydis.h>

using namespace std;

namespace iw4x
{
  static constexpr int32_t min_hook_size (14);
  static constexpr int32_t max_hook_size (32);
  static constexpr int64_t max_hook_disp (INT32_MAX);

  namespace
  {
    // Check if the memory region is committed and accessible.
    //
    bool
    is_readable (void* addr, size_t length)
    {
      MEMORY_BASIC_INFORMATION mbi;
      uint8_t* p (static_cast<uint8_t*> (addr));
      uint8_t* e (p + length);

      while (p < e)
      {
        if (!VirtualQuery (p, &mbi, sizeof (mbi)))
          return false;

        if (mbi.State != MEM_COMMIT ||
            (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
          return false;

        // Move to the next memory region.
        //
        p = static_cast<uint8_t*> (mbi.BaseAddress) + mbi.RegionSize;
      }

      return true;
    }

    // Allocate a "trampoline" page within the +/- 2GB range of the target.
    //
    // x86-64 RIP-relative instructions only have a signed 32-bit displacement.
    // To successfully relocate instructions from the original function to our
    // trampoline, the trampoline must be "close enough" to the original code
    // so that any relative references remain representable.
    //
    void*
    alloc_trampoline (uintptr_t target, size_t size)
    {
      SYSTEM_INFO si;
      GetSystemInfo (&si);

      size_t ag (si.dwAllocationGranularity);
      size_t as (size + min_hook_size);

      // Helper to clamp the search window.
      //
      auto bounded = [] (uintptr_t a, size_t d, bool above) -> uintptr_t
      {
        if (above)
          return a < UINTPTR_MAX - d ? a + d : UINTPTR_MAX;
        else
          return a > d ? a - d : 0;
      };

      uintptr_t min_addr (bounded (target, max_hook_disp, false));
      uintptr_t max_addr (bounded (target, max_hook_disp, true));

      // Probe outward from the target address in chunks of allocation
      // granularity.
      //
      for (size_t off (0); off < max_hook_disp; off += ag)
      {
        // Try allocating above the target.
        //
        if (target + off <= max_addr)
        {
          uintptr_t p ((target + off + ag - 1) & ~(ag - 1));
          void* m (VirtualAlloc (reinterpret_cast<void*> (p),
                                 as,
                                 MEM_COMMIT | MEM_RESERVE,
                                 PAGE_EXECUTE_READWRITE));

          if (m != nullptr)
          {
            auto d (static_cast<int64_t> (reinterpret_cast<uintptr_t> (m)) -
                    static_cast<int64_t> (target));

            if (d >= -max_hook_disp && d <= max_hook_disp)
              return m;

            VirtualFree (m, 0, MEM_RELEASE);
          }
        }

        // Try allocating below the target.
        //
        if (off > 0 && target >= off && target - off >= min_addr)
        {
          uintptr_t p ((target - off) & ~(ag - 1));
          void* m (VirtualAlloc (reinterpret_cast<void*> (p),
                                 as,
                                 MEM_COMMIT | MEM_RESERVE,
                                 PAGE_EXECUTE_READWRITE));

          if (m != nullptr)
          {
            auto d (static_cast<int64_t> (reinterpret_cast<uintptr_t> (m)) -
                    static_cast<int64_t> (target));

            if (d >= -max_hook_disp && d <= max_hook_disp)
              return m;

            VirtualFree (m, 0, MEM_RELEASE);
          }
        }
      }

      return nullptr;
    }

    // Write an absolute indirect jump (JMP [RIP+0]; <addr>) to the buffer.
    //
    void
    write_jump (uint8_t* buf, const void* dest)
    {
      ZydisEncoderRequest r {};
      r.mnemonic = ZYDIS_MNEMONIC_JMP;
      r.machine_mode = ZYDIS_MACHINE_MODE_LONG_64;
      r.operand_count = 1;

      ZydisEncoderOperand* o (&r.operands[0]);
      o->type = ZYDIS_OPERAND_TYPE_MEMORY;
      o->mem.base = ZYDIS_REGISTER_RIP;
      o->mem.displacement = 0;
      o->mem.size = sizeof (ZyanU64);

      ZyanU8 instr[ZYDIS_MAX_INSTRUCTION_LENGTH];
      ZyanUSize len (sizeof (instr));

      if (ZYAN_FAILED (ZydisEncoderEncodeInstruction (&r, instr, &len)))
        throw runtime_error ("unable to encode jump instruction");

      // The instruction is 6 bytes (FF 25 00 00 00 00) and the absolute address
      // follows immediately.
      //
      memmove (buf, instr, len);
      memmove (buf + len, &dest, sizeof (dest));
    }
  }

  void
  detour (void*& target, void* hook)
  {
    ZydisDecoder d {};
    ZydisDecoderInit (&d, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);

    ZydisDecodedInstruction ins[max_hook_size];
    ZydisDecodedOperand ops[max_hook_size][ZYDIS_MAX_OPERAND_COUNT];

    // Decode instructions at the target until we have cleared enough space
    // for our 14-byte jump.
    //
    size_t ds (0); // Decoded size (bytes)
    size_t di (0); // Decoded instruction count

    auto t_ptr (static_cast<uint8_t*> (target));

    while (ds < min_hook_size && di < max_hook_size)
    {
      if (!is_readable (t_ptr + ds, ZYDIS_MAX_INSTRUCTION_LENGTH))
        throw runtime_error ("instruction straddles unreadable boundary");

      if (ZYAN_FAILED (ZydisDecoderDecodeFull (&d,
                                               t_ptr + ds,
                                               ZYDIS_MAX_INSTRUCTION_LENGTH,
                                               &ins[di],
                                               ops[di])))
        throw runtime_error ("unable to decode instruction");

      // Check for function terminators.
      //
      // If we hit a return or an unconditional branch before we've secured
      // enough space for our hook, it means the function is too small.
      //
      // If we proceed, we would be overwriting whatever comes next (padding,
      // data, or the next function).
      //
      if (ds + ins[di].length < min_hook_size)
      {
        ZydisMnemonic m (ins[di].mnemonic);

        // RET (Near/Far)
        //
        if (m == ZYDIS_MNEMONIC_RET || m == ZYDIS_MNEMONIC_IRET)
          throw runtime_error ("function too small: hit return instruction");

        // INT3 (Padding/Break)
        //
        if (m == ZYDIS_MNEMONIC_INT3)
          throw runtime_error ("function too small: hit padding (int3)");

        // Unconditional JMP (but not CALL)
        //
        // Note: We only care if it's a "terminal" jump.
        //
        if (m == ZYDIS_MNEMONIC_JMP)
        {
          // Check if it's a direct relative jump (E9 or EB).
          //
          if (ops [di][0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
              ops [di][0].imm.is_relative)
          {
            ZyanU64 target_addr;
            ZydisCalcAbsoluteAddress (&ins [di],
                                      &ops [di][0],
                                      reinterpret_cast<ZyanU64> (t_ptr + ds),
                                      &target_addr);

            // "Short" jumps are usually local.
            //
            // If the jump is an 8-bit relative displacement (opcode EB), it is
            // extremely likely to be an intra-function skip (e.g., jumping over
            // a small data block or optimized else-branch).
            //
            // We might choose to risk overwriting the "gap" bytes if we trust
            // they aren't the start of a new function packed tightly against
            // this one.
            //
            // That said, note that 32-bit jumps (E9) are frequently used for
            // tail-calls to other functions, so we treat them as fatal.
            //
            bool is_short (ins [di].raw.imm [0].size == 8);

            if (!is_short)
              throw runtime_error (
                "function too small: hit near jump (tail call?)");

            // If it is short, we proceed. But be warned: we are now overwriting
            // the bytes *skipped* by this jump. If those bytes are actually
            // data used by the code at the jump target (e.g. `LEA RAX,
            // [RIP-5]`), we will corrupt them.
            //
          }
          else
          {
            // Indirect JMP (FF 25, FF E0, etc).
            //
            // These are almost always Thunks (jumps to Import Address Table)
            // or switch tables. The function effectively ends here.
            //
            throw runtime_error (
              "function too small: hit indirect jump (thunk)");
          }
        }
      }

      ds += ins[di].length;
      di++;
    }

    // Allocate the trampoline.
    //
    auto t_addr (reinterpret_cast<uintptr_t> (target));
    void* frame (alloc_trampoline (t_addr, ds));

    if (frame == nullptr)
      throw runtime_error ("unable to allocate isolated address frame");

    auto f_ptr (static_cast<uint8_t*> (frame));
    auto f_addr (reinterpret_cast<uintptr_t> (frame));

    // Relocate the stolen instructions into the trampoline.
    //
    size_t rd (0);          // Relocated data size
    uintptr_t ip (t_addr);  // Current address in source

    for (size_t i (0); i < di; ++i)
    {
      ZydisEncoderRequest r {};
      ZydisDecodedInstruction* ri (&ins[i]);
      ZydisDecodedOperand* ro (ops[i]);
      ZyanU8 rv (ri->operand_count_visible);

      // Convert decoded instruction back to a request so we can modify it.
      //
      if (ZYAN_FAILED (ZydisEncoderDecodedInstructionToEncoderRequest (ri,
                                                                       ro,
                                                                       rv,
                                                                       &r)))
        throw runtime_error ("unable to create encoder request");

      // Fix RIP-relative addressing and Relative Immediates.
      //
      // We need to handle two cases where the instruction encodes a distance
      // from the current IP:
      //
      // 1. Memory operands relative to RIP (e.g., LEA RAX, [RIP+0x100]).
      // 2. Relative immediate operands (e.g., CALL 0x100, JMP 0x20).
      //
      // In both cases, the "value" is a delta. Since we moved the instruction,
      // the delta to the (static) target address has changed.
      //
      for (ZyanU8 n (0); n < rv; ++n)
      {
        // RIP-relative memory addressing.
        //
        if (ro[n].type == ZYDIS_OPERAND_TYPE_MEMORY &&
            ro[n].mem.base == ZYDIS_REGISTER_RIP)
        {
          int64_t old_disp (ro[n].mem.disp.value);
          int64_t target_abs (ip + ri->length + old_disp);
          int64_t new_disp (target_abs - (f_addr + rd + ri->length));

          if (!in_range<int32_t> (new_disp))
            throw out_of_range ("RIP-relative displacement out of range");

          r.operands[n].mem.displacement = new_disp;
        }

        // Relative immediates (Branches).
        //
        if (ro[n].type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
                 ro[n].imm.is_relative)
        {
          int64_t old_disp (ro[n].imm.value.s);
          int64_t target_abs (ip + ri->length + old_disp);
          int64_t new_disp (target_abs - (f_addr + rd + ri->length));

          // We must make sure the new displacement fits into the operand's
          // original bit-width.
          //
          // Note: This often fails for 8-bit relative jumps (JMP SHORT) if the
          // trampoline is allocated far away. We cannot upgrade short jumps to
          // near jumps here because it would change the instruction length,
          // breaking the offsets for subsequent instructions in the block.
          //
          switch (ro[n].size)
          {
          case 8:
            if (!in_range<int8_t> (new_disp))
              throw out_of_range ("8-bit relative branch out of range");
            break;
          case 16:
            if (!in_range<int16_t> (new_disp))
              throw out_of_range ("16-bit relative branch out of range");
            break;
          case 32:
            if (!in_range<int32_t> (new_disp))
              throw out_of_range ("32-bit relative branch out of range");
            break;
          }

          r.operands[n].imm.s = new_disp;
        }
      }

      ZyanUSize len (ds + min_hook_size - rd);

      if (ZYAN_FAILED (ZydisEncoderEncodeInstruction (&r, f_ptr + rd, &len)))
        throw runtime_error ("unable to encode relocated instruction");

      rd += len;
      ip += ri->length;
    }

    // Append a jump back to the original function (after the stolen bytes).
    //
    write_jump (f_ptr + rd, t_ptr + ds);

    // Overwrite the original function with a jump to our hook.
    //
    write_jump (t_ptr, hook);

    // Update the caller's pointer to point to the trampoline (the "original"
    // function logic).
    //
    target = frame;
  }
}
