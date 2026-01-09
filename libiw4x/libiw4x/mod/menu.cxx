#include <libiw4x/mod/menu.hxx>

namespace iw4x
{
  namespace mod
  {
    namespace
    {
      menuDef_t&
      find_menu (const string& name)
      {
        XAssetHeader header (DB_FindXAssetHeader (ASSET_TYPE_MENU,
                                                  name.c_str()));

        // FIXME: DB_FindXAssetHeader will deadlock if the
        //        menu can't be found.
        //
        return *header.menu;
      }

      itemDef_s&
      find_item (menuDef_t& m, const string& name)
      {
        for (int i (0); i < m.itemCount; ++i)
        {
          itemDef_s* d (m.items [i]);

          // The engine does not guarantee invariants. Validate pointers before
          // comparing names.
          //
          if ((d != nullptr) && (d->text != nullptr) && name == d->text)
            return *d;
        }

        // At this point, we failed to locate the item, which mean that either
        // the menu had no items, or the requested item was not found. It's
        // unclear whether a menu is allowed to exist without items (and if so,
        // why find_item would be called), but let's handle both cases
        // explicitly.
        //
        throw runtime_error (m.itemCount == 0 ? "menu item count is empty"
                                              : "menu item not found: " + name);
      }

      MenuEventHandler*
      make_handler (const string& cmd)
      {
        MenuEventHandler* h (new MenuEventHandler ());

        h->eventType = 0;
        h->eventData.unconditionalScript = _strdup (cmd.c_str ());

        return h;
      }

      MenuEventHandlerSet*
      make_handler_set (const vector<string>& cmds)
      {
        MenuEventHandlerSet* hs (new MenuEventHandlerSet ());

        hs->eventHandlerCount = static_cast<int>  (cmds.size ());
        hs->eventHandlers = new MenuEventHandler* [cmds.size ()];

        for (size_t i (0); i != cmds.size (); ++i)
          hs->eventHandlers[i] = make_handler (cmds[i]);

        return hs;
      }
    }

    void
    menu ()
    {
      ctx.sched.post ("com_frame", [] ()
      {
        // Helper: Replace the action of a menu item with a custom script.
        //
        auto action = [] (const string& menu,
                          const string& item,
                          const vector<string>& cmds)
        {
          menuDef_t& m (find_menu (menu));
          itemDef_s& i (find_item (m, item));

          i.action = make_handler_set (cmds);
        };

        // Helper: Clear the disabled expression of an item to force it
        // enabled.
        //
        auto expression = [] (const string& menu, const string& item)
        {
          menuDef_t& m (find_menu (menu));
          itemDef_s& i (find_item (m, item));

          i.disabledExp = nullptr;
        };

        // Enable "Play Online".
        //
        // Normally requires GDK checks, but we bypass that and simply open the
        // xboxlive menu.
        //
        action ("main_text",
                "@PLATFORM_PLAY_ONLINE_CAPS",
                {
                  "play mouse_click",
                  "open menu_xboxlive"
                });

        // Configure Private Match.
        //
        // We need to execute a sequence of setup commands so that server state
        // is correct for a private lobby.
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

        // Unlock lobby.
        //
        // Force-enable the "Start Game" and "Game Setup" buttons in the private
        // lobby, as the engine thinks we don't have permissions.
        //
        expression ("menu_xboxlive_privatelobby", "@MENU_START_GAME_CAPS");
        expression ("menu_xboxlive_privatelobby", "@MENU_GAME_SETUP_CAPS");
      });
    }
  }
}
