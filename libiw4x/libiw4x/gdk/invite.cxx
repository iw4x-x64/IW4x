#include <libiw4x/gdk/invite.hxx>

#include <libiw4x/logger.hxx>

#include <libiw4x/gdk/async.hxx>
#include <libiw4x/gdk/error.hxx>
#include <libiw4x/gdk/handler.hxx>
#include <libiw4x/gdk/runtime.hxx>
#include <libiw4x/gdk/argument.hxx>

using namespace std;

namespace iw4x
{
  namespace gdk
  {
    namespace
    {
      struct invite_handler
      {
        void* context;
        void (*callback) (void*, const char*);
      };

      handler_table<invite_handler>&
      invite_handlers () noexcept
      {
        static handler_table<invite_handler> t;
        return t;
      }

      class invitation: public event
      {
      public:
        invitation (const invite_handler& h, const activation& u) noexcept
            : handler_ (h), uri_ (u) {}

        void
        deliver () override
        {
          info ("handing the game an invitation, {} characters", uri_.size ());

          handler_.callback (handler_.context, uri_.c_str ());

          info ("the game has taken the invitation");
        }

      private:
        invite_handler handler_;
        activation     uri_;
      };
    }

    activation
    activation_uri (string_view c) noexcept
    {
      activation r ("ms-xbl-multiplayer://activity?connectionString=");

      char        b[activation_capacity];
      text_writer w (b, sizeof (b));

      for (unsigned char x: c)
      {
        if ((x >= 'a' && x <= 'z') ||
            (x >= 'A' && x <= 'Z') ||
            (x >= '0' && x <= '9') ||
            x == '-' || x == '_' || x == '.' || x == '~')
        {
          w.write (static_cast<char> (x));
          continue;
        }

        w.write (string_view ("%"));

        static const char digits[] = "0123456789ABCDEF";

        w.write (digits[x >> 4]);
        w.write (digits[x & 0xF]);
      }

      return activation ("ms-xbl-multiplayer://activity?connectionString={}",
                         string_view (b, w.size ()));
    }

    void
    deliver_invite (string_view c) noexcept
    {
      if (!initialized ())
      {
        warn ("an invitation arrived before the gaming runtime was up");
        return;
      }

      activation u (activation_uri (c));

      invite_handler hs[handler_table<invite_handler>::capacity];

      size_t n (invite_handlers ().live (hs, sizeof (hs) / sizeof (*hs)));

      for (size_t i (0); i != n; ++i)
      {
        if (!post (nullptr, make<invitation> (hs[i], u)))
          warn ("no queue for an invitation, which was dropped");
      }
    }

    HRESULT WINAPI xgame_invite::
    register_for_event (void*,
                        void*,
                        void* context,
                        void (*callback) (void*, const char*),
                        uint64_t* token) noexcept
    {
      return guard ("XGameInviteRegisterForEvent", [&] () -> HRESULT
      {
        if (callback == nullptr)
          raise_invalid ("no callback");

        uint64_t t (
          invite_handlers ().add (invite_handler {context, callback}));

        if (t == 0)
          raise (E_OUTOFMEMORY, "no room for another invite handler");

        answer (token) = t;

        l1 ("invite handler {} registered", t);
        return S_OK;
      });
    }

    char WINAPI xgame_invite::
    unregister_for_event (void*, uint64_t token, bool) noexcept
    {
      return guard ("XGameInviteUnregisterForEvent", char (0), [&] () -> char
      {
        if (!invite_handlers ().remove (token))
          return 0;

        l1 ("invite handler {} unregistered", token);
        return 1;
      });
    }
  }
}
