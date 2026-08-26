#include <libiw4x/gdk/networking.hxx>

#include <libiw4x/logger.hxx>

#include <libiw4x/gdk/error.hxx>
#include <libiw4x/gdk/handler.hxx>
#include <libiw4x/gdk/argument.hxx>

namespace iw4x
{
  namespace gdk
  {
    namespace
    {
      constexpr std::uint32_t ethernet (6);

      constexpr connectivity_hint hint
      {
        static_cast<std::uint32_t> (connectivity_level::internet_access),
        ethernet,
        static_cast<std::uint32_t> (connectivity_cost::unrestricted),
        true,
        false,
        false,
        false
      };

      struct connectivity_handler
      {
        void* context;
        void (*callback) (void*, const connectivity_hint*);
      };

      handler_table<connectivity_handler>&
      connectivity_handlers () noexcept
      {
        static handler_table<connectivity_handler> t;
        return t;
      }

      class notification: public event
      {
      public:
        explicit
        notification (const connectivity_handler& h) noexcept: handler_ (h) {}

        void
        deliver () override
        {
          handler_.callback (handler_.context, &hint);
        }

      private:
        connectivity_handler handler_;
      };

      class security_operation: public operation
      {
      public:
        std::size_t
        work () override
        {
          return 0;
        }
      };

      const operation_id security_id
      {
        "XNetworkingQuerySecurityInformationForUrlAsync"
      };
    }

    HRESULT WINAPI xnetworking::
    query_security_information_async (void*,
                                      const void*,
                                      async_block* b) noexcept
    {
      return guard (security_id.name, [&] () -> HRESULT
      {
        l1 ("no pinning information for this url");

        begin (b, security_id, make<security_operation> ());
        return S_OK;
      });
    }

    HRESULT WINAPI xnetworking::
    query_security_information_result_size (void*,
                                            async_block* b,
                                            std::size_t* out) noexcept
    {
      return guard ("XNetworkingQuerySecurityInformationForUrlResultSize",
                    [&] () -> HRESULT
      {
        return result_size (b, out);
      });
    }

    HRESULT WINAPI xnetworking::
    query_security_information_result (void*,
                                       async_block* b,
                                       std::size_t size,
                                       std::size_t* used,
                                       void*,
                                       void** out) noexcept
    {
      return guard ("XNetworkingQuerySecurityInformationForUrlResult",
                    [&] () -> HRESULT
      {
        HRESULT hr (result (b, security_id, size, nullptr));

        if (FAILED (hr))
          return hr;

        if (out != nullptr)
          *out = nullptr;

        if (used != nullptr)
          *used = 0;

        return S_OK;
      });
    }

    HRESULT WINAPI xnetworking::
    verify_server_certificate (void*, void*, const void* information) noexcept
    {
      return guard ("XNetworkingVerifyServerCertificate", [&] () -> HRESULT
      {
        if (information != nullptr)
          warn ("server certificate presented with pinning information we "
                "did not supply");

        return S_OK;
      });
    }

    HRESULT WINAPI xnetworking::
    get_connectivity_hint (void*, connectivity_hint* out) noexcept
    {
      return guard ("XNetworkingGetConnectivityHint", [&] () -> HRESULT
      {
        l1 ("XNetworkingGetConnectivityHint");

        answer (out) = hint;
        return S_OK;
      });
    }

    HRESULT WINAPI xnetworking::
    register_connectivity_changed (
      void*,
      void*,
      void* context,
      void (*callback) (void*, const connectivity_hint*),
      std::uint64_t* token) noexcept
    {
      return guard ("XNetworkingRegisterConnectivityHintChanged",
                    [&] () -> HRESULT
      {
        if (callback == nullptr)
          raise_invalid ("no callback");

        connectivity_handler h {context, callback};

        std::uint64_t t (connectivity_handlers ().add (h));

        if (t == 0)
          raise (E_OUTOFMEMORY, "no room for another connectivity handler");

        answer (token) = t;

        l1 ("connectivity handler {} registered", t);

        if (!post (nullptr, make<notification> (h)))
          warn ("no queue for the initial connectivity notification");

        return S_OK;
      });
    }

    HRESULT WINAPI xnetworking::
    unregister_connectivity_changed (void*,
                                     std::uint64_t token,
                                     bool) noexcept
    {
      return guard ("XNetworkingUnregisterConnectivityHintChanged",
                    [&] () -> HRESULT
      {
        if (!connectivity_handlers ().remove (token))
          raise_invalid ("no connectivity handler {}", token);

        l1 ("connectivity handler {} unregistered", token);
        return S_OK;
      });
    }
  }
}
