#include <libiw4x/mod/mod-ui.hxx>

#include <libiw4x/log/log-formatter.hxx>

namespace iw4x
{
  namespace mod
  {
    namespace
    {
      XAssetHeader
      find_asset (XAssetType t, const string& n, unsigned int to = 5)
      {
        log::trace_l2 {categories::ui {},
                       "searching for asset '{}' (type {})",
                       n, t};

        // DB_FindXAssetHeader will deadlock if the requested asset is missing,
        // so we route the call through our "timeout" proxy.
        //
        // Note that we must capture 'n' by value. If we captured by reference,
        // a timeout would return execution to the caller, destroying the string
        // while the background thread is potentially still trying to read it.
        //
        future<XAssetHeader> f (async (launch::async,
                                       [t, n] ()
        {
          return DB_FindXAssetHeader (t, n.c_str ());
        }));

        future_status s (f.wait_for (seconds (to)));

        if (s == future_status::timeout)
        {
          // Bail out. The background thread is likely permanently stuck holding
          // the engine lock, but at least we can survive and diagnose it here
          // without a hard freeze.
          //
          log::error {categories::ui {},
                      "timeout ({}s) searching for asset type {}: {}",
                      to, t, n};

          throw runtime_error ("asset search timeout");
        }

        log::trace_l2 {categories::ui {}, "found asset '{}'", n};

        return f.get ();
      }

      menuDef_t&
      find_menu (const string& n)
      {
        log::trace_l1 {categories::ui {}, "locating menu '{}'", n};

        // We rely on the engine's asset database to locate the menu definition.
        //
        // Because find_asset will throw on a timeout (which is our proxy for
        // missing assets due to the engine's deadlock bug), reaching the next
        // line implicitly means we have a valid pointer.
        //
        XAssetHeader h (find_asset (ASSET_TYPE_MENU, n));

        return *h.menu;
      }

      itemDef_s&
      find_item (menuDef_t& m, const string& mn, const string& in)
      {
        log::trace_l1 {categories::ui {},
                       "scanning items in menu '{}' for '{}'",
                       mn, in};

        // Menus store their items in a flat array of pointers. We iterate the
        // list linearly to match the requested text label.
        //
        for (int i (0); i < m.itemCount; ++i)
        {
          itemDef_s* d (m.items[i]);

          // Engine invariants here are surprisingly weak. Even if itemCount is
          // greater than zero, individual item slots might be null. The text
          // field can also be null. Validate everything before comparing.
          //
          if (d != nullptr && d->text != nullptr && in == d->text)
          {
            log::trace_l3 {categories::ui {},
                           "found item '{}' at index {}",
                           in, i};
            return *d;
          }
        }

        // If we fall through, the item doesn't exist.
        //
        string r (m.itemCount == 0
                  ? "menu '" + mn + "' has no items"
                  : "menu '" + mn + "' item '" + in + "' not found");

        log::error {categories::ui {}, "item lookup failed: {}", r};

        throw runtime_error (r);
      }

      MenuEventHandler*
      make_eh (const string& c)
      {
        log::trace_l3 {categories::ui {},
                       "creating event handler for script '{}'",
                       c};

        MenuEventHandler* h (new MenuEventHandler ());

        h->eventType = 0;

        // Ownership semantics for the script string are opaque. The engine
        // expects a raw C string and appears to read it on the fly during event
        // dispatch without ever freeing it. We allocate a copy and effectively
        // leak it here.
        //
        h->eventData.unconditionalScript = _strdup (c.c_str ());

        return h;
      }

      MenuEventHandlerSet*
      make_ehs (const vector<string>& cs)
      {
        log::trace_l2 {categories::ui {},
                       "creating event handler set ({} events)",
                       cs.size ()};

        MenuEventHandlerSet* s (new MenuEventHandlerSet ());

        s->eventHandlerCount = static_cast<int> (cs.size ());
        s->eventHandlers = new MenuEventHandler* [cs.size ()];

        // Populate our handler array. Note that if make_eh throws (say, an
        // OOM), we leak whatever we've allocated so far. Given that we are OOM,
        // the game is dead anyway, so we don't bother with RAII cleanup here.
        //
        for (size_t i (0); i != cs.size (); ++i)
          s->eventHandlers[i] = make_eh (cs[i]);

        return s;
      }
    }

    ui_module::
    ui_module ()
    {
      scheduler::post (com_frame_domain {}, [] ()
      {
        try
        {
          // Helper to overwrite a button's script action.
          //
          auto act ([] (const string& mn,
                        const string& in,
                        const vector<string>& cs)
          {
            menuDef_t& m (find_menu (mn));
            itemDef_s& i (find_item (m, mn, in));

            // We blindly overwrite the action. Any old handler set attached to
            // this item is leaked. The engine lacks a garbage collector for
            // orphaned UI handlers, but this happens only once at startup.
            //
            i.action = make_ehs (cs);
          });

          act
          (
            "main_text",
            "@PLATFORM_PLAY_ONLINE_CAPS",
            {
              "play mouse_click",

              "open menu_xboxlive"
            }
          );
        }
        catch (const runtime_error& e)
        {
          log::critical {categories::ui {},
                         "failed to override ui: {}",
                         e.what ()};
          exit (1);
        }
      });
    }
  }
}
