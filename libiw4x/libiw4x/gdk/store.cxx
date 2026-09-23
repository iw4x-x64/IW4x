#include <libiw4x/gdk/store.hxx>

#include <libiw4x/logger.hxx>

#include <libiw4x/gdk/error.hxx>
#include <libiw4x/gdk/argument.hxx>

using namespace std;

namespace iw4x
{
  namespace gdk
  {
    namespace
    {
      store_context&
      context () noexcept
      {
        static store_context c;
        return c;
      }

      const operation_id license_id {"XStoreQueryGameLicenseAsync"};

      class license_operation: public operation
      {
      public:
        size_t
        work () override
        {
          return sizeof (game_license);
        }

        void
        result (size_t size, void* buffer) override
        {
          if (size < sizeof (game_license))
            raise_invalid ("no room for a game licence");

          game_license& l (answer (static_cast<game_license*> (buffer)));

          l = game_license {};
          l.active = true;
          l.trial = false;

          info ("game licence reported active");
        }
      };
    }

    HRESULT WINAPI xstore::
    create_context (void*, void* user, store_context** out) noexcept
    {
      return guard ("XStoreCreateContext", [&] () -> HRESULT
      {
        l1 ("store context created for user {}", user);

        ++context ().open;

        answer (out) = &context ();
        return S_OK;
      });
    }

    void WINAPI xstore::
    close_context_handle (void*, store_context* c) noexcept
    {
      guard ("XStoreCloseContextHandle", [&] () -> void
      {
        if (c != nullptr && c->open > 0)
          --c->open;

        l1 ("store context closed, {} outstanding", context ().open);
      });
    }

    HRESULT WINAPI xstore::
    query_game_license_async (void*,
                              store_context* c,
                              async_block* b) noexcept
    {
      return guard (license_id.name, [&] () -> HRESULT
      {
        if (c == nullptr)
          raise_invalid ("no store context");

        begin (b, license_id, make_unique<license_operation> ());
        return S_OK;
      });
    }

    HRESULT WINAPI xstore::
    query_game_license_result (void*,
                               async_block* b,
                               game_license* out) noexcept
    {
      return guard ("XStoreQueryGameLicenseResult", [&] () -> HRESULT
      {
        return result (b, license_id, sizeof (game_license), &answer (out));
      });
    }
  }
}
