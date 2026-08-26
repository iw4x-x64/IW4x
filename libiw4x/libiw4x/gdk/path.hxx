#pragma once

#include <cstdint>
#include <cstddef>

#include <windows.h>

#include <libiw4x/gdk/text.hxx>

namespace iw4x
{
  namespace gdk
  {
    class path
    {
    public:
      static constexpr std::size_t capacity = 512;

      path () noexcept
      {
        value_[0] = L'\0';
      }

      explicit
      path (const wchar_t* p) noexcept
      {
        value_[0] = L'\0';

        extend (p);
      }

      const wchar_t*
      c_str () const noexcept
      {
        return value_;
      }

      std::size_t
      size () const noexcept
      {
        return size_;
      }

      bool
      empty () const noexcept
      {
        return size_ == 0;
      }

      bool
      whole () const noexcept
      {
        return !truncated_;
      }

      void
      clear () noexcept
      {
        size_ = 0;
        truncated_ = false;
        value_[0] = L'\0';
      }

      bool
      extend (const wchar_t*) noexcept;

      bool
      extend (const wchar_t*, std::size_t) noexcept;

      bool
      extend (chars) noexcept;

      bool
      append (const wchar_t*) noexcept;

      bool
      append (chars) noexcept;

      void
      to_directory () noexcept;

      chars
      narrow (char* buffer, std::size_t size) const noexcept;

    private:
      std::size_t size_ = 0;
      bool        truncated_ = false;
      wchar_t     value_[capacity];
    };

    bool
    is_directory (const path&) noexcept;

    bool
    create_directories (const path&) noexcept;

    class blob
    {
    public:
      blob () noexcept = default;

      blob (const blob&) = delete;
      blob& operator= (const blob&) = delete;

      blob (blob&& b) noexcept: data_ (b.data_), size_ (b.size_)
      {
        b.data_ = nullptr;
        b.size_ = 0;
      }

      blob&
      operator= (blob&& b) noexcept
      {
        if (this != &b)
        {
          clear ();

          data_ = b.data_;
          size_ = b.size_;

          b.data_ = nullptr;
          b.size_ = 0;
        }

        return *this;
      }

      ~blob ()
      {
        clear ();
      }

      void
      clear () noexcept;

      bool
      resize (std::size_t) noexcept;

      std::uint8_t*
      data () noexcept
      {
        return data_;
      }

      const std::uint8_t*
      data () const noexcept
      {
        return data_;
      }

      std::size_t
      size () const noexcept
      {
        return size_;
      }

      bool
      empty () const noexcept
      {
        return size_ == 0;
      }

    private:
      std::uint8_t* data_ = nullptr;
      std::size_t   size_ = 0;
    };

    class file
    {
    public:
      file () noexcept = default;

      file (const file&) = delete;
      file& operator= (const file&) = delete;

      ~file ()
      {
        close ();
      }

      bool
      open_read (const path&) noexcept;

      bool
      open_write (const path&) noexcept;

      void
      close () noexcept;

      bool
      opened () const noexcept
      {
        return handle_ != INVALID_HANDLE_VALUE;
      }

      bool
      size (std::uint64_t&) const noexcept;

      std::int64_t
      last_write_seconds () const noexcept;

      bool
      read (void*, std::size_t) noexcept;

      bool
      write (const void*, std::size_t) noexcept;

    private:
      HANDLE handle_ = INVALID_HANDLE_VALUE;
    };

    class directory
    {
    public:
      directory () noexcept = default;

      directory (const directory&) = delete;
      directory& operator= (const directory&) = delete;

      ~directory ()
      {
        close ();
      }

      bool
      open (const path&) noexcept;

      void
      close () noexcept;

      bool
      next () noexcept;

      const wchar_t*
      name () const noexcept
      {
        return entry_.cFileName;
      }

      bool
      is_directory () const noexcept
      {
        return (entry_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
      }

      std::uint64_t
      size () const noexcept
      {
        return (static_cast<std::uint64_t> (entry_.nFileSizeHigh) << 32) |
               entry_.nFileSizeLow;
      }

      std::int64_t
      last_write_seconds () const noexcept;

    private:
      HANDLE          handle_ = INVALID_HANDLE_VALUE;
      WIN32_FIND_DATAW entry_ {};
      bool            first_ = false;
    };

    std::int64_t
    to_seconds (const FILETIME&) noexcept;
  }
}
