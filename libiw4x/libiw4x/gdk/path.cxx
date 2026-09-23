#include <libiw4x/gdk/path.hxx>

#include <new>
#include <limits>
#include <cwchar>
#include <cstring>

#include <libiw4x/contract.hxx>

using namespace std;

namespace iw4x
{
  namespace gdk
  {
    bool path::
    extend (const wchar_t* p, size_t n) noexcept
    {
      LIBIW4X_INVARIANT (size_ < capacity && value_[size_] == L'\0');

      if (p == nullptr || n == 0)
        return whole ();

      if (n > capacity - size_ - 1)
      {
        n = capacity - size_ - 1;
        truncated_ = true;
      }

      memcpy (value_ + size_, p, n * sizeof (wchar_t));

      size_ += n;
      value_[size_] = L'\0';

      return whole ();
    }

    bool path::
    extend (const wchar_t* p) noexcept
    {
      if (p == nullptr)
        return whole ();

      return extend (p, wcslen (p));
    }

    bool path::
    extend (string_view p) noexcept
    {
      if (p.empty ())
        return whole ();

      wchar_t w[capacity];

      int n (MultiByteToWideChar (CP_UTF8,
                                  0,
                                  p.data (),
                                  static_cast<int> (p.size ()),
                                  w,
                                  static_cast<int> (capacity)));

      if (n <= 0)
      {
        truncated_ = true;
        return false;
      }

      return extend (w, static_cast<size_t> (n));
    }

    bool path::
    append (const wchar_t* p) noexcept
    {
      if (size_ != 0 && value_[size_ - 1] != L'\\')
        extend (L"\\", 1);

      return extend (p);
    }

    bool path::
    append (string_view p) noexcept
    {
      if (size_ != 0 && value_[size_ - 1] != L'\\')
        extend (L"\\", 1);

      return extend (p);
    }

    void path::
    to_directory () noexcept
    {
      LIBIW4X_INVARIANT (size_ < capacity && value_[size_] == L'\0');

      while (size_ != 0 && value_[size_ - 1] != L'\\')
        --size_;

      while (size_ > 1 && value_[size_ - 1] == L'\\')
        --size_;

      value_[size_] = L'\0';
    }

    string_view path::
    narrow (char* b, size_t n) const noexcept
    {
      if (n == 0)
        return string_view ();

      LIBIW4X_PRE (b != nullptr);
      LIBIW4X_PRE (n - 1 <= static_cast<size_t> (numeric_limits<int>::max ()));

      int r (WideCharToMultiByte (CP_UTF8,
                                  0,
                                  value_,
                                  static_cast<int> (size_),
                                  b,
                                  static_cast<int> (n - 1),
                                  nullptr,
                                  nullptr));

      if (r < 0)
        r = 0;

      LIBIW4X_ASSERT (static_cast<size_t> (r) < n);

      b[r] = '\0';

      return string_view (b, static_cast<size_t> (r));
    }

    string_view
    narrow (const wchar_t* w, char* b, size_t n) noexcept
    {
      if (n == 0)
        return string_view ();

      LIBIW4X_PRE (w != nullptr && b != nullptr);
      LIBIW4X_PRE (n <= static_cast<size_t> (numeric_limits<int>::max ()));

      int r (WideCharToMultiByte (CP_UTF8,
                                  0,
                                  w,
                                  -1,
                                  b,
                                  static_cast<int> (n),
                                  nullptr,
                                  nullptr));

      if (r <= 0)
      {
        b[0] = '\0';
        return string_view ();
      }

      LIBIW4X_ASSERT (static_cast<size_t> (r) <= n);

      return string_view (b, static_cast<size_t> (r) - 1);
    }

    namespace
    {
      bool
      make_directory (const wchar_t* p) noexcept
      {
        if (CreateDirectoryW (p, nullptr) != 0)
          return true;

        if (GetLastError () != ERROR_ALREADY_EXISTS)
          return false;

        DWORD a (GetFileAttributesW (p));

        return a != INVALID_FILE_ATTRIBUTES &&
               (a & FILE_ATTRIBUTE_DIRECTORY) != 0;
      }
    }

    bool
    is_directory (const path& p) noexcept
    {
      DWORD a (GetFileAttributesW (p.c_str ()));

      return a != INVALID_FILE_ATTRIBUTES &&
             (a & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    bool
    create_directories (const path& p) noexcept
    {
      path w;

      const wchar_t* s (p.c_str ());

      for (size_t i (1); i != p.size (); ++i)
      {
        if (s[i] != L'\\' && s[i] != L'/')
          continue;

        w.clear ();
        w.extend (s, i);

        if (w.size () != 0 && w.c_str ()[w.size () - 1] != L':')
          make_directory (w.c_str ());
      }

      return make_directory (p.c_str ());
    }

    int64_t
    to_seconds (const FILETIME& t) noexcept
    {
      uint64_t v ((static_cast<uint64_t> (t.dwHighDateTime) << 32) |
                  t.dwLowDateTime);

      return static_cast<int64_t> (v / 10000000ULL) - 11644473600LL;
    }

    void blob::
    clear () noexcept
    {
      delete[] data_;

      data_ = nullptr;
      size_ = 0;
    }

    bool blob::
    resize (size_t n) noexcept
    {
      if (n == size_)
        return true;

      clear ();

      if (n == 0)
        return true;

      data_ = new (nothrow) uint8_t[n];

      if (data_ == nullptr)
        return false;

      size_ = n;
      return true;
    }

    bool file::
    open_read (const path& p) noexcept
    {
      close ();

      handle_ = CreateFileW (p.c_str (),
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             nullptr,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             nullptr);

      return opened ();
    }

    bool file::
    open_write (const path& p) noexcept
    {
      close ();

      handle_ = CreateFileW (p.c_str (),
                             GENERIC_WRITE,
                             0,
                             nullptr,
                             CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL,
                             nullptr);

      return opened ();
    }

    void file::
    close () noexcept
    {
      if (opened ())
        CloseHandle (handle_);

      handle_ = INVALID_HANDLE_VALUE;
    }

    bool file::
    size (uint64_t& n) const noexcept
    {
      LIBIW4X_PRE (opened ());

      LARGE_INTEGER v;

      if (!GetFileSizeEx (handle_, &v))
        return false;

      n = static_cast<uint64_t> (v.QuadPart);
      return true;
    }

    int64_t file::
    last_write_seconds () const noexcept
    {
      LIBIW4X_PRE (opened ());

      FILETIME t;

      if (!GetFileTime (handle_, nullptr, nullptr, &t))
        return 0;

      return to_seconds (t);
    }

    bool file::
    read (void* b, size_t n) noexcept
    {
      LIBIW4X_PRE (opened ());
      LIBIW4X_PRE (b != nullptr || n == 0);

      auto* p (static_cast<uint8_t*> (b));

      while (n != 0)
      {
        DWORD c (static_cast<DWORD> (n > 0x10000000 ? 0x10000000 : n));
        DWORD r (0);

        if (!ReadFile (handle_, p, c, &r, nullptr) || r == 0)
          return false;

        p += r;
        n -= r;
      }

      return true;
    }

    bool file::
    write (const void* b, size_t n) noexcept
    {
      LIBIW4X_PRE (opened ());
      LIBIW4X_PRE (b != nullptr || n == 0);

      auto* p (static_cast<const uint8_t*> (b));

      while (n != 0)
      {
        DWORD c (static_cast<DWORD> (n > 0x10000000 ? 0x10000000 : n));
        DWORD w (0);

        if (!WriteFile (handle_, p, c, &w, nullptr) || w == 0)
          return false;

        p += w;
        n -= w;
      }

      return true;
    }

    bool directory::
    open (const path& p) noexcept
    {
      close ();

      path w (p);

      w.append (L"*");

      handle_ = FindFirstFileW (w.c_str (), &entry_);

      if (handle_ == INVALID_HANDLE_VALUE)
        return false;

      first_ = true;
      return true;
    }

    void directory::
    close () noexcept
    {
      if (handle_ != INVALID_HANDLE_VALUE)
        FindClose (handle_);

      handle_ = INVALID_HANDLE_VALUE;
      first_ = false;
    }

    bool directory::
    next () noexcept
    {
      if (handle_ == INVALID_HANDLE_VALUE)
        return false;

      for (;;)
      {
        if (first_)
          first_ = false;
        else if (FindNextFileW (handle_, &entry_) == 0)
          return false;

        const wchar_t* n (entry_.cFileName);

        if (n[0] == L'.' &&
            (n[1] == L'\0' || (n[1] == L'.' && n[2] == L'\0')))
          continue;

        return true;
      }
    }

    int64_t directory::
    last_write_seconds () const noexcept
    {
      return to_seconds (entry_.ftLastWriteTime);
    }
  }
}
