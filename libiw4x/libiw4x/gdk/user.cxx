#include <libiw4x/gdk/user.hxx>

#include <atomic>

#include <libiw4x/logger.hxx>

#include <libiw4x/gdk/error.hxx>
#include <libiw4x/gdk/sync.hxx>
#include <libiw4x/gdk/handler.hxx>
#include <libiw4x/gdk/argument.hxx>

namespace iw4x
{
  namespace gdk
  {
    namespace
    {
      struct local_user
      {
        std::atomic<unsigned> references {1};
      };

      local_user&
      the_user () noexcept
      {
        static local_user u;
        return u;
      }

      local_user&
      user_of (void* h)
      {
        if (h != &the_user ())
          raise_invalid ("not a user handle of ours");

        return the_user ();
      }

      const operation_id add_id {"XUserAddAsync"};

      class add_operation: public operation
      {
      public:
        explicit
        add_operation (std::uint32_t options) noexcept: options_ (options) {}

        std::size_t
        work () override
        {
          the_user ().references.fetch_add (1, std::memory_order_relaxed);

          info ("user added, options {:#x}", options_);

          return sizeof (local_user*);
        }

        void
        result (std::size_t size, void* buffer) override
        {
          if (size < sizeof (local_user*))
            raise_invalid ("no room for a user handle");

          answer (static_cast<local_user**> (buffer)) = &the_user ();
        }

      private:
        std::uint32_t options_;
      };

      const operation_id resolve_id {"XUserResolvePrivilegeWithUiAsync"};

      class resolve_operation: public operation
      {
      public:
        std::size_t
        work () override
        {
          return 0;
        }
      };

      struct audience
      {
        mutex             mutex_;
        service_predicate claims[max_claimants] {};
        unsigned          claimants = 0;
      };

      audience&
      claimed () noexcept
      {
        static audience a;
        return a;
      }

      bool
      served (chars method, chars url) noexcept
      {
        audience&  a (claimed ());
        scope_lock l (a.mutex_);

        for (unsigned i (0); i != a.claimants; ++i)
        {
          if (a.claims[i] (method, url))
            return true;
        }

        return false;
      }

      inline constexpr std::size_t token_capacity (256);

      const operation_id token_id {"XUserGetTokenAndSignatureAsync"};

      class token_operation: public operation
      {
      public:
        std::size_t
        work () override
        {
          char n[gamertag_capacity];

          std::size_t k (gamertag (n, sizeof (n)));

          token_ = text<token_capacity> ("IW4x1.0 xuid={} gamertag={}",
                                         hex (xuid (), 16),
                                         chars (n, k));

          return sizeof (token_and_signature) + token_.size () + 1;
        }

        void
        result (std::size_t size, void* buffer) override
        {
          std::size_t n (sizeof (token_and_signature) + token_.size () + 1);

          if (buffer == nullptr)
            raise (E_POINTER, "no buffer");

          if (size < n)
            raise (insufficient_buffer,
                   "a {} byte buffer holds none of a {} byte token",
                   size,
                   n);

          auto* d (static_cast<token_and_signature*> (buffer));
          auto* p (reinterpret_cast<char*> (d + 1));

          __builtin_memcpy (p, token_.c_str (), token_.size () + 1);

          d->token_size = token_.size ();
          d->token = p;

          d->signature_size = 0;
          d->signature = "";
        }

      private:
        text<token_capacity> token_;
      };

      struct change_handler
      {
        void* context;
        void (*callback) (void*, std::uint64_t, std::uint32_t);
      };

      handler_table<change_handler>&
      change_handlers () noexcept
      {
        static handler_table<change_handler> t;
        return t;
      }
    }

    void
    serve (service_predicate p) noexcept
    {
      audience&  a (claimed ());
      scope_lock l (a.mutex_);

      if (p == nullptr)
        return;

      for (unsigned i (0); i != a.claimants; ++i)
      {
        if (a.claims[i] == p)
          return;
      }

      if (a.claimants == max_claimants)
      {
        warn ("no room for another service claimant, {} in use",
              max_claimants);

        return;
      }

      a.claims[a.claimants++] = p;
    }

    unsigned
    claimant_count () noexcept
    {
      audience&  a (claimed ());
      scope_lock l (a.mutex_);

      return a.claimants;
    }

    HRESULT WINAPI xuser::
    duplicate_handle (void*, void* u, void** out) noexcept
    {
      return guard ("XUserDuplicateHandle", [&] () -> HRESULT
      {
        local_user& r (user_of (u));

        l1 ("user handle duplicated, {} outstanding",
            r.references.fetch_add (1, std::memory_order_relaxed) + 1);

        answer (out) = &r;
        return S_OK;
      });
    }

    HRESULT WINAPI xuser::
    close_handle (void*, void* u) noexcept
    {
      return guard ("XUserCloseHandle", [&] () -> HRESULT
      {
        l1 ("user handle closed, {} outstanding",
            user_of (u).references.fetch_sub (1, std::memory_order_acq_rel) -
              1);

        return S_OK;
      });
    }

    std::int32_t WINAPI xuser::
    compare (void*, void* a, void* b) noexcept
    {
      if (a == b)
        return 0;

      return a < b ? -1 : 1;
    }

    HRESULT WINAPI xuser::
    add_async (void*, std::uint32_t options, async_block* b) noexcept
    {
      return guard (add_id.name, [&] () -> HRESULT
      {
        l1 ("XUserAddAsync (options {:#x})", options);

        begin (b, add_id, make<add_operation> (options));
        return S_OK;
      });
    }

    HRESULT WINAPI xuser::
    add_result (void*, async_block* b, void** out) noexcept
    {
      return guard ("XUserAddResult", [&] () -> HRESULT
      {
        return result (b, add_id, sizeof (local_user*), &answer (out));
      });
    }

    HRESULT WINAPI xuser::
    get_id (void*, void* u, std::uint64_t* out) noexcept
    {
      return guard ("XUserGetId", [&] () -> HRESULT
      {
        user_of (u);

        answer (out) = xuid ();

        l1 ("XUserGetId -> {:#x}", *out);
        return S_OK;
      });
    }

    HRESULT WINAPI xuser::
    get_local_id (void*, void* u, std::uint64_t* out) noexcept
    {
      return guard ("XUserGetLocalId", [&] () -> HRESULT
      {
        user_of (u);

        answer (out) = 1;
        return S_OK;
      });
    }

    HRESULT WINAPI xuser::
    get_state (void*, void* u, user_state* out) noexcept
    {
      return guard ("XUserGetState", [&] () -> HRESULT
      {
        user_of (u);

        answer (out) = user_state::signed_in;
        return S_OK;
      });
    }

    HRESULT WINAPI xuser::
    get_age_group (void*, void* u, age_group* out) noexcept
    {
      return guard ("XUserGetAgeGroup", [&] () -> HRESULT
      {
        user_of (u);

        answer (out) = age_group::adult;
        return S_OK;
      });
    }

    HRESULT WINAPI xuser::
    get_is_guest (void*, void* u, bool* out) noexcept
    {
      return guard ("XUserGetIsGuest", [&] () -> HRESULT
      {
        user_of (u);

        answer (out) = false;
        return S_OK;
      });
    }

    HRESULT WINAPI xuser::
    check_privilege (void*,
                     void* u,
                     std::uint32_t options,
                     std::uint32_t privilege,
                     bool* r,
                     deny_reason* reason) noexcept
    {
      return guard ("XUserCheckPrivilege", [&] () -> HRESULT
      {
        l1 ("XUserCheckPrivilege (privilege {}, options {:#x})",
            privilege,
            options);

        user_of (u);

        answer (r) = true;

        if (reason != nullptr)
          *reason = deny_reason::none;

        return S_OK;
      });
    }

    HRESULT WINAPI xuser::
    resolve_privilege_async (void*,
                             void* u,
                             std::uint32_t options,
                             std::uint32_t privilege,
                             async_block* b) noexcept
    {
      return guard (resolve_id.name, [&] () -> HRESULT
      {
        l1 ("privilege {} resolution requested (options {:#x}), nothing to "
            "resolve",
            privilege,
            options);

        user_of (u);

        begin (b, resolve_id, make<resolve_operation> ());
        return S_OK;
      });
    }

    HRESULT WINAPI xuser::
    resolve_privilege_result (void*, async_block* b) noexcept
    {
      return guard ("XUserResolvePrivilegeWithUiResult", [&] () -> HRESULT
      {
        return result (b, resolve_id, 0, nullptr);
      });
    }

    HRESULT WINAPI xuser::
    get_token_and_signature_async (void*,
                                   void* u,
                                   std::uint32_t options,
                                   const char* method,
                                   const char* url,
                                   std::size_t,
                                   const void*,
                                   std::size_t,
                                   const void*,
                                   async_block* b) noexcept
    {
      return guard (token_id.name, [&] () -> HRESULT
      {
        chars m (method != nullptr ? method : "");
        chars x (url != nullptr ? url : "");

        l1 ("token requested for {} {} (options {:#x})",
            m.data (),
            x.data (),
            options);

        user_of (u);

        if (!served (m, x))
        {
          l1 ("no token for {}, which is not a service of ours", x.data ());
          return E_NOTIMPL;
        }

        begin (b, token_id, make<token_operation> ());
        return S_OK;
      });
    }

    HRESULT WINAPI xuser::
    get_token_and_signature_result_size (void*,
                                         async_block* b,
                                         std::size_t* out) noexcept
    {
      return guard ("XUserGetTokenAndSignatureResultSize", [&] () -> HRESULT
      {
        return result_size (b, out);
      });
    }

    HRESULT WINAPI xuser::
    get_token_and_signature_result (void*,
                                    async_block* b,
                                    std::size_t size,
                                    void* buffer,
                                    void** out,
                                    std::size_t* used) noexcept
    {
      return guard ("XUserGetTokenAndSignatureResult", [&] () -> HRESULT
      {
        l1 ("XUserGetTokenAndSignatureResult (size {})", size);

        HRESULT hr (result (b, token_id, size, buffer));

        if (FAILED (hr))
          return hr;

        answer (out) = buffer;

        if (used != nullptr)
        {
          auto* d (static_cast<token_and_signature*> (buffer));

          *used = sizeof (token_and_signature) + d->token_size + 1;
        }

        return S_OK;
      });
    }

    HRESULT WINAPI xuser::
    register_for_change (void*,
                         void*,
                         void* context,
                         void (*callback) (void*,
                                           std::uint64_t,
                                           std::uint32_t),
                         std::uint64_t* token) noexcept
    {
      return guard ("XUserRegisterForChangeEvent", [&] () -> HRESULT
      {
        if (callback == nullptr)
          raise_invalid ("no callback");

        std::uint64_t t (
          change_handlers ().add (change_handler {context, callback}));

        if (t == 0)
          raise (E_OUTOFMEMORY, "no room for another user change handler");

        answer (token) = t;

        l1 ("user change handler {} registered", t);
        return S_OK;
      });
    }

    HRESULT WINAPI xuser::
    unregister_for_change (void*, std::uint64_t token, bool) noexcept
    {
      return guard ("XUserUnregisterForChangeEvent", [&] () -> HRESULT
      {
        if (!change_handlers ().remove (token))
          raise_invalid ("no user change handler {}", token);

        l1 ("user change handler {} unregistered", token);
        return S_OK;
      });
    }

    HRESULT WINAPI xuser_gamertag::
    get_gamertag (void*,
                  void* u,
                  gamertag_component c,
                  std::size_t size,
                  char* buffer,
                  std::size_t* used) noexcept
    {
      return guard ("XUserGetGamertag", [&] () -> HRESULT
      {
        l1 ("XUserGetGamertag (component {}, size {})",
            static_cast<std::uint32_t> (c),
            size);

        user_of (u);

        std::size_t bound (0);

        if (!component_bound (c, bound))
          raise_invalid ("unrecovered gamertag component {}",
                         static_cast<std::uint32_t> (c));

        char n[gamertag_capacity];

        std::size_t k (bound != 0 ? gamertag (n, bound + 1) : 0);

        copy_out (chars (n, k), size, buffer, used);
        return S_OK;
      });
    }
  }
}
