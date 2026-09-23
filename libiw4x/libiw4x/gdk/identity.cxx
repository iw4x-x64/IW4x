#include <libiw4x/gdk/identity.hxx>

#include <charconv>

#include <windows.h>
#include <bcrypt.h>

#include <libiw4x/logger.hxx>

#include <libiw4x/gdk/error.hxx>
#include <libiw4x/gdk/sync.hxx>
#include <libiw4x/gdk/storage.hxx>

using namespace std;

namespace iw4x
{
  namespace gdk
  {
    namespace
    {
      constexpr uint64_t xuid_shape (0x0009000000000000ULL);

      path
      identity_file ()
      {
        path r (storage_root ());

        r.append (L"identity");

        return r;
      }

      bool
      read_identity (uint64_t& v) noexcept
      {
        file f;

        if (!f.open_read (identity_file ()))
          return false;

        char b[32];

        uint64_t n (0);

        if (!f.size (n) || n == 0)
          return false;

        if (n > sizeof (b))
          n = sizeof (b);

        if (!f.read (b, static_cast<size_t> (n)))
          return false;

        string_view s (b, static_cast<size_t> (n));

        if (s.starts_with ("0x") || s.starts_with ("0X"))
        {
          s.remove_prefix (2);
        }

        uint64_t r (0);

        if (from_chars (s.data (), s.data () + s.size (), r, 16).ec != errc () ||
            r == 0)
        {
          return false;
        }

        v = r;
        return true;
      }

      uint64_t
      random_identity ()
      {
        uint64_t v (0);

        NTSTATUS s (BCryptGenRandom (nullptr,
                                     reinterpret_cast<PUCHAR> (&v),
                                     sizeof (v),
                                     BCRYPT_USE_SYSTEM_PREFERRED_RNG));

        if (s < 0)
          raise (E_FAIL, "unable to draw a local user id, status {}",
                 hex (static_cast<uint32_t> (s), 8));

        return xuid_shape | (v & 0x0000FFFFFFFFFFFFULL);
      }

      uint64_t
      mint_identity ()
      {
        uint64_t v (random_identity ());

        text<32> s ("{}", hex (v, 16));

        file f;

        if (!f.open_write (identity_file ()) ||
            !f.write (s.c_str (), s.size ()))
          raise_win32 ("unable to write the local user id");

        return v;
      }

      size_t
      bounded (const char* s, size_t n, size_t limit) noexcept
      {
        if (n <= limit)
          return n;

        while (limit != 0 &&
               (static_cast<unsigned char> (s[limit]) & 0xC0) == 0x80)
          --limit;

        return limit;
      }

      struct local_name
      {
        mutex  mutex_;
        char   value[gamertag_capacity];
        size_t size = 0;
      };

      local_name&
      name () noexcept
      {
        static local_name n;
        return n;
      }
    }

    uint64_t
    xuid ()
    {
      static const uint64_t v (
        [] () -> uint64_t
        {
          uint64_t r (0);

          if (read_identity (r))
          {
            info ("local user {:#x}", r);
            return r;
          }

          r = mint_identity ();

          info ("local user {:#x} minted", r);
          return r;
        } ());

      return v;
    }

    size_t
    gamertag (char* b, size_t size) noexcept
    {
      if (b == nullptr || size == 0)
        return 0;

      local_name& n (name ());
      scope_lock  l (n.mutex_);

      if (n.size == 0)
      {
        text<gamertag_capacity> d (
          "IW4x-{}",
          hex (static_cast<uint32_t> (xuid () & 0xFFFFFFFFULL), 8));

        n.size = d.size ();

        __builtin_memcpy (n.value, d.c_str (), n.size);
      }

      size_t k (bounded (n.value, n.size, size - 1));

      __builtin_memcpy (b, n.value, k);

      b[k] = '\0';
      return k;
    }

    void
    set_gamertag (string_view v) noexcept
    {
      local_name& n (name ());
      scope_lock  l (n.mutex_);

      size_t k (bounded (v.data (), v.size (), gamertag_capacity - 1));

      if (string_view (n.value, n.size) == v.substr (0, k))
        return;

      v.copy (n.value, k);

      n.value[k] = '\0';
      n.size = k;

      info ("local user name is {}", n.value);
    }

    bool
    component_bound (gamertag_component c, size_t& n) noexcept
    {
      switch (c)
      {
      case gamertag_component::classic:       n = 15;  return true;
      case gamertag_component::modern:        n = 96;  return true;
      case gamertag_component::modern_suffix: n = 0;   return true;
      case gamertag_component::unique_modern: n = 100; return true;
      }

      return false;
    }
  }
}
