#include <libiw4x/mod/mod-ui.hxx>

#include <algorithm>
#include <ranges>

#include <libiw4x/detour.hxx>
#include <libiw4x/scheduler.hxx>

using namespace std;

namespace iw4x
{
  namespace mod
  {
    namespace
    {
      menuDef_t&
      find_menu (const char* name)
      {
        XAssetHeader h (DB_FindXAssetHeader (ASSET_TYPE_MENU, name));

        // FIXME: DB_FindXAssetHeader will deadlock if the
        //        asset can't be found.
        //
        return *h.menu;
      }

      itemDef_s&
      find_item (menuDef_t& m, const char* name)
      {
        for (int i (0); i < m.itemCount; ++i)
        {
          itemDef_s* d (m.items [i]);

          // The engine's UI structures provide zero pointer safety guarantees.
          // We have to validate the item and its text pointer before doing our
          // string comparison to avoid crashing on malformed assets.
          //
          if (d != nullptr && d->text != nullptr && strcmp (name, d->text) == 0)
            return *d;
        }

        // If we drop through the loop, the item isn't there. This usually means
        // the menu structure is out of sync with our expectations. It's also
        // possible the menu legitimately has no items, though why we'd search
        // an empty menu is another question.
        //
        exit (1);
      }

      MenuEventHandler*
      make_event_handler (const char* script)
      {
        MenuEventHandler* h (new MenuEventHandler ());

        h->eventType = 0;
        h->eventData.unconditionalScript = _strdup (script);

        return h;
      }

      MenuEventHandlerSet*
      make_event_handler_set (initializer_list<const char*> scripts)
      {
        MenuEventHandlerSet* hs (new MenuEventHandlerSet ());

        hs->eventHandlerCount = static_cast<int> (scripts.size ());
        hs->eventHandlers = new MenuEventHandler* [scripts.size ()];

        ranges::transform (scripts, hs->eventHandlers, make_event_handler);

        return hs;
      }

      void
      set_action (const char* menu,
                  const char* item,
                  initializer_list<const char*> scripts)
      {
        menuDef_t& m (find_menu (menu));
        itemDef_s& i (find_item (m, item));

        i.action = make_event_handler_set (scripts);
      }
    }

    ui_module::
    ui_module ()
    {
      scheduler::post (com_frame_domain, []
      {
        set_action
        (
          "main_text",
          "@PLATFORM_PLAY_ONLINE_CAPS",
          {
            "play mouse_click",
            "execnow nosplitscreen",
            "exec xrequirelivesignin",
            "setdvar systemlink 0",
            "setdvar splitscreen 0",
            "setdvar onlinegame 1",
            "exec default_xboxlive.cfg",
            "exec party_maxplayers 9",
            "exec xblive_privatematch 0",
            "exec xblive_rankedmatch 0",
            "exec xstartprivateparty",
            "setdvar ui_mptype 0",
            "open menu_xboxlive"
          }
        );
      });
    }
  }
}
