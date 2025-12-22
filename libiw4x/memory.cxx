#include <libiw4x/memory.hxx>

namespace iw4x
{
  static constexpr array<array<uint8_t, 9>, 9> nop_sequences
  {{
    {{0x90}},
    {{0x66, 0x90}},
    {{0x0F, 0x1F, 0x00}},
    {{0x0F, 0x1F, 0x40, 0x00}},
    {{0x0F, 0x1F, 0x44, 0x00, 0x00}},
    {{0x66, 0x0F, 0x1F, 0x44, 0x00, 0x00}},
    {{0x0F, 0x1F, 0x80, 0x00, 0x00, 0x00, 0x00}},
    {{0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {{0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00}}
  }};

  // @@: We could eventually snapshot the executable with all "write" patches
  // pre-applied to avoid doing the work at runtime. Note that if we do this,
  // any dynamic patches (for example s_cpuCount) must be explicitly excluded
  // and still applied at runtime, since their values can't be baked into a
  // snapshot.
  //
  void*
  memwrite (void* dest, int ch, std::size_t count)
  {
    // Zero-length operation is no-op by definition (includes nullptr case).
    //
    if (count == 0)
      return dest;

    void* r (nullptr);

    // Treat 0x90 as a request for architectural NOP sequences.
    //
    if (ch == 0x90)
    {
      uint8_t* d (reinterpret_cast<uint8_t*> (dest));

      // Use 9-byte sequence as the bulk filler.
      //
      const uint8_t* s (nop_sequences[8].data ());

      size_t q (count / 9);
      for (size_t i (0); i != q; ++i)
      {
        memcpy (d, s, 9);
        d += 9;
      }

      // Any tail < 9 bytes is satisfied by the corresponding canonical form.
      //
      size_t rem (count % 9);
      if (rem != 0)
        memcpy (d, nop_sequences[rem - 1].data (), rem);

      r = dest;
    }
    else
    {
      // For all non-NOP cases, fall back to the ordinary byte blast.
      //
      r = std::memset (dest, ch, count);
    }

    return r;
  }

  void*
  memwrite (uintptr_t dest, int ch, size_t count)
  {
    return memwrite (reinterpret_cast<void*> (dest), ch, count);
  }

  void*
  memwrite (void* dest, const void* src, size_t count)
  {
    // Zero-length operation is no-op by definition (includes nullptr case).
    //
    if (count == 0)
      return dest;

    const uint8_t* s (static_cast<const uint8_t*> (src));
    uint8_t* d (static_cast<uint8_t*> (dest));

    for (size_t i (0); i < count;)
    {
      if (s [i] == 0x90)
      {
        size_t n (0);
        while (i + n < count && s [i + n] == 0x90)
          n++;

        memwrite (d + i, 0x90, n);
        i += n;
      }
      else
      {
        size_t n (0);
        while (i + n < count && s [i + n] != 0x90)
          n++;

        memcpy (d + i, s + i, n);
        i += n;
      }
    }

    return dest;
  }

  void*
  memwrite (uintptr_t dest, const void* src, size_t count)
  {
    return memwrite (reinterpret_cast<void*> (dest), src, count);
  }
}
