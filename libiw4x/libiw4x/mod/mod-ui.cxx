#include <libiw4x/mod/mod-ui.hxx>

namespace iw4x
{
  namespace mod
  {
    namespace
    {
      // TODO: Move this to database.
      //

      XAssetHeader
      db_find_xasset_header (XAssetType t, const string& n, unsigned int to = 5)
      {
        // Launch the lookup in the background to protect the main thread.
        //
        // The engine's DB_FindXAssetHeader deadlock if the asset pipeline is
        // stalling or if the menu is missing entirely during the load phase.
        //
        // Note also that we must capture 'n' by value. That is, if we
        // captured by reference and hit the timeout, we would return and
        // destroy the string while the background thread is still reading it,
        // causing a use-after-free segfault.
        //
        future<XAssetHeader> f (async (launch::async,
                                       [t, n] ()
        {
          return DB_FindXAssetHeader (t, n.c_str ());
        }));

        // Block and wait for the result up to the limit.
        //
        future_status s (f.wait_for (seconds (to)));

        if (s == future_status::timeout)
        {
          // We timed out. The background thread is likely wedged in the engine
          // lock.
          //
          ostringstream s;
          s << "timeout (" << to << "s) searching for asset type "
             << t << ": " << n;

          throw runtime_error (s.str ());
        }

        return f.get ();
      }

      menuDef_t&
      find_menu (const string& n)
      {
        // We rely on the engine's asset database to locate the menu
        // definition.
        //
        // As noted in db_find_xasset_header, the engine implementation is
        // broken: it does not return nullptr if an asset is missing; it
        // deadlocks trying to load it.
        //
        // Therefore, if this call returns without throwing a timeout, we know
        // implicitly that the asset was found and the pointer is valid.
        //
        XAssetHeader h (db_find_xasset_header (ASSET_TYPE_MENU, n.c_str ()));

        return *h.menu;
      }

      itemDef_s&
      find_item (menuDef_t& m, const string& mn, const string& in)
      {
        // The menu definition stores items in a flat array of pointers. We
        // have to iterate the list manually to find the matching text label.
        //
        for (int i (0); i < m.itemCount; ++i)
        {
          itemDef_s* d (m.items[i]);

          // The engine does not guarantee invariants for these pointers. Even
          // if itemCount says we have items, slots can be null. We must also
          // validate the text field before comparison to prevent segfaults.
          //
          if (d != nullptr && d->text != nullptr && in == d->text)
            return *d;
        }

        // If we are here, the scan failed. We include both the menu name (mn)
        // and item name (in) in the diagnostic. This is critical because menu
        // structures often change between patches or mods, and we need to know
        // exactly what went missing.
        //
        throw runtime_error (m.itemCount == 0
                             ? "menu '" + mn + "' has no items"
                             : "menu '" + mn + "' item '" + in + "' not found");
      }

      MenuEventHandler*
      make_handler (const string& c)
      {
        MenuEventHandler* h (new MenuEventHandler ());

        // Ownership of this string is murky. The engine expects a raw char*
        // buffer for the script, which it presumably reads during event
        // dispatch.
        //
        h->eventType = 0;
        h->eventData.unconditionalScript = _strdup (c.c_str ());

        return h;
      }

      MenuEventHandlerSet*
      make_handler_set (const vector<string>& cs)
      {
        MenuEventHandlerSet* hs (new MenuEventHandlerSet ());

        hs->eventHandlerCount = static_cast<int> (cs.size ());
        hs->eventHandlers = new MenuEventHandler* [cs.size ()];

        // Populate the handler array. If any make_handler call throws (bad
        // alloc), we leak the previous ones and the set itself (in that OOM
        // scenario, the game is dead anyway).
        //
        for (size_t i (0); i != cs.size (); ++i)
          hs->eventHandlers[i] = make_handler (cs[i]);

        return hs;
      }
    }

    ui::
    ui ()
    {
      ctx.sched.post ("com_frame", [] ()
      {
        try
        {
          auto action = [] (const string& m,
                            const string& i,
                            const vector<string>& cs)
          {
            menuDef_t& md (find_menu (m));
            itemDef_s& id (find_item (md, m, i));

            // We are overwriting the existing action. The old handler set is
            // leaked here unless the engine has some garbage collection for
            // orphaned UI handlers (unlikely).
            //
            id.action = make_handler_set (cs);
          };

          auto expression = [] (const string& m, const string& i)
          {
            menuDef_t& md (find_menu (m));
            itemDef_s& id (find_item (md, m, i));

            // Setting this to null effectively removes the "conditional" check
            // for the button, making it always clickable.
            //
            id.disabledExp = nullptr;
          };

          action ("main_text",
                  "@PLATFORM_PLAY_ONLINE_CAPS",
                  {
                    "play mouse_click",
                    "exec xsignin;",
                    "open menu_xboxlive"
                  });

          // Configure Private Match.
          //
          action ("menu_xboxlive",
                  "@MENU_PRIVATE_MATCH_CAPS",
                  {
                    "play mouse_click",
                    "exec xcheckezpatch",
                    "exec default_xboxlive.cfg",
                    "exec xblive_rankedmatch 0",
                    "exec ui_enumeratesaved exec xblive_privatematch 1",
                    "exec onlinegame 0",
                    "exec xblive_hostingprivateparty 1",
                    "exec xblive_privatepartyclient 1",
                    "exec xstartprivatematch",
                    "open menu_xboxlive_privatelobby"
                  });

          // Unlock lobby buttons.
          //
          expression ("menu_xboxlive_privatelobby", "@MENU_START_GAME_CAPS");
          expression ("menu_xboxlive_privatelobby", "@MENU_GAME_SETUP_CAPS");
        }
        catch (const runtime_error& e)
        {
          cerr << "error: unable to patch menu: " << e.what () << endl;
        }
      });
    }
  }
}
