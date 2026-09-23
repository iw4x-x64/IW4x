#include <libiw4x/gdk/save.hxx>

#include <utility>

#include <libiw4x/logger.hxx>

#include <libiw4x/gdk/sync.hxx>
#include <libiw4x/gdk/error.hxx>
#include <libiw4x/gdk/storage.hxx>
#include <libiw4x/gdk/argument.hxx>
#include <libiw4x/gdk/identity.hxx>

using namespace std;

namespace iw4x
{
  namespace gdk
  {
    bool
    storable (string_view n) noexcept
    {
      if (n.empty () || n == "." || n == "..")
        return false;

      for (char c: n)
      {
        switch (c)
        {
        case '/':
        case '\\':
        case ':':
        case '*':
        case '?':
        case '"':
        case '<':
        case '>':
        case '|':
          return false;
        }
      }

      return true;
    }

    namespace
    {
      string_view
      storable_name (const char* n)
      {
        string_view v (n != nullptr ? n : "");

        if (!storable (v))
          raise_invalid ("'{}' is not a name this store can use", v);

        return v;
      }

      template <typename T, size_t N>
      class handle_pool
      {
      public:
        T*
        open () noexcept
        {
          scope_lock l (mutex_);

          for (size_t i (0); i != N; ++i)
          {
            if (used_[i])
              continue;

            items_[i] = T ();
            used_[i] = true;

            return &items_[i];
          }

          return nullptr;
        }

        T&
        lookup (void* h)
        {
          scope_lock l (mutex_);

          for (size_t i (0); i != N; ++i)
          {
            if (used_[i] && &items_[i] == h)
              return items_[i];
          }

          raise_invalid ("not a handle of ours");
        }

        bool
        close (void* h) noexcept
        {
          scope_lock l (mutex_);

          for (size_t i (0); i != N; ++i)
          {
            if (!used_[i] || &items_[i] != h)
              continue;

            items_[i] = T ();
            used_[i] = false;

            return true;
          }

          return false;
        }

      private:
        mutable mutex mutex_;

        T    items_[N];
        bool used_[N] {};
      };

      save_provider&
      the_provider () noexcept
      {
        static save_provider p;
        return p;
      }

      save_provider&
      provider_of (void* h)
      {
        if (h != &the_provider ())
          raise_invalid ("not a game save provider of ours");

        return the_provider ();
      }

      handle_pool<save_container, max_containers>&
      containers () noexcept
      {
        static handle_pool<save_container, max_containers> t;
        return t;
      }

      handle_pool<save_update, max_updates>&
      updates () noexcept
      {
        static handle_pool<save_update, max_updates> t;
        return t;
      }

      const operation_id initialize_id {"XGameSaveInitializeProviderAsync"};

      class initialize_operation: public operation
      {
      public:
        size_t
        work () override
        {
          path r (storage_root ());

          r.append (L"cloud");
          r.append (text<32> ("{}", hex (xuid (), 16)));

          if (!r.whole ())
            raise (E_FAIL, "the game save root does not fit in a path");

          if (!create_directories (r))
            raise_win32 ("unable to create the game save root");

          the_provider ().root = r;

          char b[path::capacity];

          info ("game save store at {}", r.narrow (b, sizeof (b)));

          return sizeof (save_provider*);
        }

        void
        result (size_t size, void* buffer) override
        {
          if (size < sizeof (save_provider*))
            raise_invalid ("no room for a provider handle");

          ++the_provider ().open;

          answer (static_cast<save_provider**> (buffer)) = &the_provider ();
        }
      };

      const operation_id read_id {"XGameSaveReadBlobDataAsync"};

      class read_operation: public operation
      {
      public:
        read_operation (const path& location,
                        const char* const* names,
                        size_t count)
            : location_ (location), count_ (count)
        {
          for (size_t i (0); i != count_; ++i)
            names_[i] = text<blob_name_capacity> ("{}",
                                                  storable_name (names[i]));
        }

        size_t
        work () override
        {
          size_t n (count_ * sizeof (save_blob));

          for (size_t i (0); i != count_; ++i)
          {
            path p (location_);

            p.append (names_[i]);

            file f;

            if (!f.open_read (p))
              raise (HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND),
                     "unable to read '{}'", names_[i].c_str ());

            uint64_t s (0);

            if (!f.size (s))
              raise_win32 ("unable to size a game save blob");

            if (!data_[i].resize (static_cast<size_t> (s)))
              raise (E_OUTOFMEMORY, "no room for a {} byte blob", s);

            if (!data_[i].empty () &&
                !f.read (data_[i].data (), data_[i].size ()))
              raise_win32 ("unable to read a game save blob");

            n += names_[i].size () + 1 + data_[i].size ();
          }

          return n;
        }

        void
        result (size_t size, void* buffer) override
        {
          auto* b (&answer (static_cast<save_blob*> (buffer)));
          auto* p (static_cast<uint8_t*> (buffer) +
                   count_ * sizeof (save_blob));
          auto* e (static_cast<uint8_t*> (buffer) + size);

          for (size_t i (0); i != count_; ++i)
          {
            const text<blob_name_capacity>& n (names_[i]);
            const blob&                     d (data_[i]);

            if (static_cast<size_t> (e - p) < n.size () + 1 + d.size ())
              raise (insufficient_buffer,
                     "no room for blob '{}'", n.c_str ());

            __builtin_memcpy (p, n.c_str (), n.size () + 1);

            b[i].info.name = reinterpret_cast<const char*> (p);
            p += n.size () + 1;

            if (!d.empty ())
            {
              __builtin_memcpy (p, d.data (), d.size ());
            }

            b[i].info.size = static_cast<uint32_t> (d.size ());
            b[i].data = p;
            p += d.size ();
          }
        }

        size_t
        count () const noexcept
        {
          return count_;
        }

      private:
        path                     location_;
        text<blob_name_capacity> names_[max_blob_reads];
        blob                     data_[max_blob_reads];
        size_t                   count_;
      };

      const operation_id submit_id {"XGameSaveSubmitUpdateAsync"};

      class submit_operation: public operation
      {
      public:
        submit_operation (const path& location,
                          pending_write* writes,
                          size_t count) noexcept
            : location_ (location), count_ (count)
        {
          for (size_t i (0); i != count_; ++i)
          {
            writes_[i].name = writes[i].name;
            writes_[i].data = move (writes[i].data);
          }
        }

        size_t
        work () override
        {
          for (size_t i (0); i != count_; ++i)
          {
            const pending_write& w (writes_[i]);

            path p (location_);
            p.append (w.name);

            path t (p);
            t.extend (L".part");

            {
              file o;

              if (!o.open_write (t))
                raise_win32 ("unable to write a game save blob");

              if (!w.data.empty () &&
                  !o.write (w.data.data (), w.data.size ()))
                raise_win32 ("unable to write a game save blob");
            }

            if (!MoveFileExW (t.c_str (),
                              p.c_str (),
                              MOVEFILE_REPLACE_EXISTING))
            {
              DeleteFileW (t.c_str ());

              raise_win32 ("unable to replace a game save blob");
            }

            info ("game save blob {} stored, {} bytes",
                  w.name.c_str (),
                  w.data.size ());
          }

          return 0;
        }

      private:
        path          location_;
        pending_write writes_[max_blob_writes];
        size_t        count_;
      };
    }

    HRESULT WINAPI xgame_save::
    initialize_provider_async (void*,
                               void* user,
                               const char* scid,
                               bool sync_on_demand,
                               async_block* b) noexcept
    {
      return guard (initialize_id.name, [&] () -> HRESULT
      {
        l1 ("XGameSaveInitializeProviderAsync for user {} scid {} sync on "
            "demand {}",
            user,
            scid != nullptr ? scid : "<none>",
            sync_on_demand);

        begin (b, initialize_id, make_unique<initialize_operation> ());
        return S_OK;
      });
    }

    HRESULT WINAPI xgame_save::
    initialize_provider_result (void*,
                                async_block* b,
                                save_provider** out) noexcept
    {
      return guard ("XGameSaveInitializeProviderResult", [&] () -> HRESULT
      {
        return result (b,
                       initialize_id,
                       sizeof (save_provider*),
                       &answer (out));
      });
    }

    void WINAPI xgame_save::
    close_provider (void*, save_provider* p) noexcept
    {
      guard ("XGameSaveCloseProvider", [&] () -> void
      {
        if (p != nullptr && p->open > 0)
          --p->open;

        l1 ("game save provider closed, {} outstanding", the_provider ().open);
      });
    }

    HRESULT WINAPI xgame_save::
    create_container (void*,
                      save_provider* p,
                      const char* name,
                      save_container** out) noexcept
    {
      return guard ("XGameSaveCreateContainer", [&] () -> HRESULT
      {
        string_view n (storable_name (name));

        path location (provider_of (p).root);

        location.append (n);

        if (!location.whole ())
          raise (E_FAIL, "a game save container name does not fit in a path");

        if (!create_directories (location))
          raise_win32 ("unable to create a game save container");

        save_container* c (containers ().open ());

        if (c == nullptr)
          raise (E_OUTOFMEMORY, "no room for another game save container");

        c->location = location;
        c->name = text<blob_name_capacity> ("{}", n);

        l1 ("game save container {} opened", c->name.c_str ());

        answer (out) = c;
        return S_OK;
      });
    }

    void WINAPI xgame_save::
    close_container (void*, save_container* c) noexcept
    {
      guard ("XGameSaveCloseContainer", [&] () -> void
      {
        containers ().close (c);
      });
    }

    HRESULT WINAPI xgame_save::
    enumerate_container_info (void*,
                              save_provider* p,
                              void* context,
                              container_info_callback cb) noexcept
    {
      return guard ("XGameSaveEnumerateContainerInfo", [&] () -> HRESULT
      {
        if (cb == nullptr)
          raise_invalid ("no callback");

        const path& root (provider_of (p).root);

        directory d;

        if (!d.open (root))
        {
          l1 ("nothing to enumerate under the game save root");
          return S_OK;
        }

        while (d.next ())
        {
          if (!d.is_directory ())
            continue;

          char  b[blob_name_capacity * 4];
          string_view n (narrow (d.name (), b, sizeof (b)));

          if (!storable (n))
            continue;

          container_info i {};
          i.name = b;
          i.display_name = b;

          path inner (root);
          inner.append (n);

          directory f;

          if (f.open (inner))
          {
            while (f.next ())
            {
              if (f.is_directory ())
                continue;

              ++i.blob_count;
              i.total_size += f.size ();

              int64_t t (f.last_write_seconds ());

              if (t > i.last_modified)
                i.last_modified = t;
            }
          }

          l1 ("game save container {} has {} blobs, modified {}",
              b,
              i.blob_count,
              i.last_modified);

          if (!cb (&i, context))
            break;
        }

        return S_OK;
      });
    }

    HRESULT WINAPI xgame_save::
    read_blob_data_async (void*,
                          save_container* c,
                          const char* const* names,
                          uint32_t count,
                          async_block* b) noexcept
    {
      return guard (read_id.name, [&] () -> HRESULT
      {
        save_container& t (containers ().lookup (c));

        if (names == nullptr)
          raise (E_POINTER, "no blob names");

        if (count > max_blob_reads)
          raise_invalid ("{} blobs is more than one read takes", count);

        for (uint32_t i (0); i != count; ++i)
          l1 ("XGameSaveReadBlobDataAsync {}/{}",
              t.name.c_str (),
              storable_name (names[i]));

        begin (b,
               read_id,
               make_unique<read_operation> (t.location, names, count));
        return S_OK;
      });
    }

    HRESULT WINAPI xgame_save::
    read_blob_data (void*,
                    async_block* b,
                    size_t size,
                    void* buffer,
                    uint32_t* count) noexcept
    {
      return guard ("XGameSaveReadBlobData", [&] () -> HRESULT
      {
        auto* o (
          static_cast<read_operation*> (pending_operation (b, read_id)));

        if (o == nullptr)
          raise_invalid ("not a blob read operation of ours");

        uint32_t n (static_cast<uint32_t> (o->count ()));

        HRESULT hr (result (b, read_id, size, buffer));

        if (SUCCEEDED (hr))
          answer (count) = n;

        return hr;
      });
    }

    HRESULT WINAPI xgame_save::
    create_update (void*,
                   save_container* c,
                   const char* display_name,
                   save_update** out) noexcept
    {
      return guard ("XGameSaveCreateUpdate", [&] () -> HRESULT
      {
        save_container& t (containers ().lookup (c));

        l1 ("XGameSaveCreateUpdate {} ({})",
            t.name.c_str (),
            display_name != nullptr ? display_name : "<none>");

        save_update* u (updates ().open ());

        if (u == nullptr)
          raise (E_OUTOFMEMORY, "no room for another game save update");

        u->target = &t;

        answer (out) = u;
        return S_OK;
      });
    }

    void WINAPI xgame_save::
    close_update (void*, save_update* u) noexcept
    {
      guard ("XGameSaveCloseUpdate", [&] () -> void
      {
        updates ().close (u);
      });
    }

    HRESULT WINAPI xgame_save::
    submit_blob_write (void*,
                       save_update* u,
                       const char* name,
                       const uint8_t* data,
                       uint32_t size) noexcept
    {
      return guard ("XGameSaveSubmitBlobWrite", [&] () -> HRESULT
      {
        save_update& t (updates ().lookup (u));

        string_view n (storable_name (name));

        if (data == nullptr && size != 0)
          raise (E_POINTER, "no blob data");

        if (t.count == max_blob_writes)
          raise (E_OUTOFMEMORY,
                 "no room for more than {} blobs in one update",
                 max_blob_writes);

        pending_write& w (t.writes[t.count]);

        w.name = text<blob_name_capacity> ("{}", n);

        if (!w.data.resize (size))
          raise (E_OUTOFMEMORY, "no room for a {} byte blob", size);

        if (size != 0)
          __builtin_memcpy (w.data.data (), data, size);

        ++t.count;

        l1 ("XGameSaveSubmitBlobWrite {}/{}, {} bytes",
            t.target->name.c_str (),
            w.name.c_str (),
            size);

        return S_OK;
      });
    }

    HRESULT WINAPI xgame_save::
    submit_update_async (void*, save_update* u, async_block* b) noexcept
    {
      return guard (submit_id.name, [&] () -> HRESULT
      {
        save_update& t (updates ().lookup (u));

        if (t.target == nullptr)
          raise_invalid ("an update with no container");

        begin (b,
               submit_id,
               make_unique<submit_operation> (t.target->location,
                                              t.writes,
                                              t.count));

        t.count = 0;

        return S_OK;
      });
    }

    HRESULT WINAPI xgame_save::
    submit_update (void*, async_block* b) noexcept
    {
      return guard ("XGameSaveSubmitUpdate", [&] () -> HRESULT
      {
        return result (b, submit_id, 0, nullptr);
      });
    }
  }
}
