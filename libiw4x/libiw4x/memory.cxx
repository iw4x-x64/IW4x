#include <libiw4x/memory.hxx>

namespace iw4x
{
  // Intel architectural NOP sequences.
  //
  // See Intel 64 and IA-32 Architectures Software Developer's Manual,
  // Volume 2B, Instruction Set Reference N-Z, Table 4-12.
  //
  // We use these instead of repeated 0x90 to reduce the pressure on the
  // instruction decoder.
  //
  static constexpr array<array<uint8_t, 9>, 9> nops
  {{
    {{0x90}},                                                 // 1
    {{0x66, 0x90}},                                           // 2
    {{0x0F, 0x1F, 0x00}},                                     // 3
    {{0x0F, 0x1F, 0x40, 0x00}},                               // 4
    {{0x0F, 0x1F, 0x44, 0x00, 0x00}},                         // 5
    {{0x66, 0x0F, 0x1F, 0x44, 0x00, 0x00}},                   // 6
    {{0x0F, 0x1F, 0x80, 0x00, 0x00, 0x00, 0x00}},             // 7
    {{0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00}},       // 8
    {{0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00}}  // 9
  }};

  void*
  memwrite (void* d, int c, size_t n)
  {
    // Zero-length operation is no-op by definition.
    //
    if (n == 0)
      return d;

    // If this is a request for 0x90 (NOP) fill, we try to be smart and use
    // the optimal multi-byte sequences.
    //
    // @@ We could eventually snapshot the executable with all "write"
    //    patches pre-applied to avoid doing this work at runtime.
    //
    if (c == 0x90)
    {
      auto p (static_cast<uint8_t*> (d));

      // We only have sequences up to 9 bytes. Fill the bulk with the largest
      // available sequence.
      //
      size_t q (n / 9);
      size_t r (n % 9);

      const uint8_t* s (nops[8].data ()); // 9-byte sequence.

      for (size_t i (0); i != q; ++i)
      {
        memcpy (p, s, 9);
        p += 9;
      }

      // Handle the tail.
      //
      if (r != 0)
        memcpy (p, nops[r - 1].data (), r);

      return d;
    }

    // For all non-NOP cases, fall back to the ordinary byte blast.
    //
    return memset (d, c, n);
  }

  void*
  memwrite (uintptr_t d, int c, size_t n)
  {
    return memwrite (reinterpret_cast<void*> (d), c, n);
  }

  void*
  memwrite (void* d, const void* s, size_t n)
  {
    if (n == 0)
      return d;

    auto pd (static_cast<uint8_t*> (d));
    auto ps (static_cast<const uint8_t*> (s));

    // Iterate over the source buffer. We are looking for runs of 0x90 which
    // we interpret as "placeholders for optimized NOPs".
    //
    // Note: This implicitly assumes the source buffer represents code. If
    // were are copying raw data where 0x90 is a valid value (e.g., offsets
    // or immediate values), this logic will corrupt it by expanding single
    // bytes into multi-byte opcodes.
    //
    for (size_t i (0); i < n;)
    {
      // If we hit a NOP placeholder, see how long the run is.
      //
      if (ps[i] == 0x90)
      {
        size_t c (0);
        while (i + c < n && ps[i + c] == 0x90)
          c++;

        // Delegate to the smart filler.
        //
        memwrite (pd + i, 0x90, c);
        i += c;
      }
      else
      {
        // Otherwise, find the next NOP or end of buffer and copy the chunk.
        //
        size_t c (0);
        while (i + c < n && ps[i + c] != 0x90)
          c++;

        memcpy (pd + i, ps + i, c);
        i += c;
      }
    }

    return d;
  }

  void*
  memwrite (uintptr_t d, const void* s, size_t n)
  {
    return memwrite (reinterpret_cast<void*> (d), s, n);
  }
}
