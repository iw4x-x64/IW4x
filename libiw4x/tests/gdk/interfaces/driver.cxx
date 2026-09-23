#undef NDEBUG
#include <cassert>

#include <cstring>
#include <string_view>

#include <libiw4x/gdk/ui.hxx>
#include <libiw4x/gdk/user.hxx>
#include <libiw4x/gdk/save.hxx>
#include <libiw4x/gdk/store.hxx>
#include <libiw4x/gdk/async.hxx>
#include <libiw4x/gdk/error.hxx>
#include <libiw4x/gdk/invite.hxx>
#include <libiw4x/gdk/vtable.hxx>
#include <libiw4x/gdk/runtime.hxx>
#include <libiw4x/gdk/registry.hxx>
#include <libiw4x/gdk/identity.hxx>
#include <libiw4x/gdk/feature.hxx>
#include <libiw4x/gdk/networking.hxx>
#include <libiw4x/gdk/unmodelled.hxx>

using namespace iw4x::gdk;

namespace
{
  task_queue* queue (nullptr);

  void
  drain ()
  {
    while ((*queue)[port::work].dispatch (0))
      ;

    while ((*queue)[port::completion].dispatch (0))
      ;
  }

  template <published_interface I>
  void*
  self ()
  {
    return const_cast<interface_object*> (&object_of<I>);
  }

  bool
  ours (std::string_view, std::string_view url)
  {
    return url.starts_with ("http:");
  }

  std::size_t
  first_two (std::string_view, const std::uint64_t* c, std::size_t n,
             std::uint64_t* out, std::size_t max)
  {
    std::size_t k (n < max ? n : max);

    if (k > 2)
      k = 2;

    for (std::size_t i (0); i != k; ++i)
      out[i] = c[i];

    return k;
  }

  int enumerated (0);

  bool
  count_container (const container_info* i, void*)
  {
    if (std::strcmp (i->name, "settings") == 0)
    {
      ++enumerated;

      assert (i->blob_count == 2);
      assert (i->total_size == 9);
      assert (i->last_modified > 1600000000);
    }

    return true;
  }
}

int
main ()
{
  queue = create_queue (dispatch_mode::manual, dispatch_mode::manual);
  assert (queue != nullptr);

  set_process_queue (queue);

  assert (InitializeApiImplEx (0, 0, 0) == S_OK);
  assert (initialized ());

  {
    assert (interface_count () == 13);

    void* o (nullptr);

    assert (QueryApiImpl (&xuser::api, &xuser::id, &o) == S_OK);
    assert (o == self<xuser> ());

    assert (QueryApiImpl (&xuser::api, &xuser_gamertag::id, &o) == S_OK);
    assert (o == self<xuser_gamertag> ());

    guid none {};
    assert (QueryApiImpl (&none, &none, &o) == E_NOINTERFACE);
    assert (QueryApiImpl (nullptr, nullptr, nullptr) == E_POINTER);
  }

  {
    auto declines ([] (const void* e, void* p) -> bool
    {
      auto d (reinterpret_cast<HRESULT (*) (void*, void*, void*, void*)> (
                const_cast<void*> (e)));

      return d (p, nullptr, nullptr, nullptr) == E_NOTIMPL;
    });

    assert (declines (slot_of<xpackage> (0x58), self<xpackage> ()));
    assert (declines (slot_of<xpackage> (0x140), self<xpackage> ()));
    assert (declines (slot_of<xgame_event> (0x18), self<xgame_event> ()));
    assert (declines (slot_of<xunidentified> (0x18),
                      self<xunidentified> ()));

    void* o (nullptr);

    assert (QueryApiImpl (&xpackage::api, &xpackage::id, &o) == S_OK);
    assert (o == self<xpackage> ());

    assert (QueryApiImpl (&xgame_event::api, &xgame_event::id, &o) == S_OK);
    assert (o == self<xgame_event> ());

    assert (QueryApiImpl (&xunidentified::api, &xunidentified::id, &o) ==
            S_OK);
    assert (o == self<xunidentified> ());

    assert (!provides (feature::package));
    assert (!provides (feature::game_event));
  }

  {
    store_context* c (nullptr);

    assert (xstore::create_context (nullptr, nullptr, &c) == S_OK);
    assert (c != nullptr);

    async_block b {};
    b.queue = queue;

    assert (xstore::query_game_license_async (nullptr, c, &b) == S_OK);
    drain ();

    game_license l {};
    assert (xstore::query_game_license_result (nullptr, &b, &l) == S_OK);
    assert (l.active && !l.trial);

    xstore::close_context_handle (nullptr, c);

    auto q (reinterpret_cast<HRESULT (*) (void*, void*, void*)> (
              const_cast<void*> (slot_of<xstore> (0x48))));

    assert (q (self<xstore> (), nullptr, nullptr) == E_NOTIMPL);

    auto p (reinterpret_cast<char (*) (void*, void*)> (
              const_cast<void*> (slot_of<xstore> (0x80))));

    assert (p (self<xstore> (), nullptr) == 0);
  }

  void* user (nullptr);

  {
    async_block b {};
    b.queue = queue;

    assert (xuser::add_async (nullptr, 0, &b) == S_OK);
    drain ();

    assert (xuser::add_result (nullptr, &b, &user) == S_OK);
    assert (user != nullptr);

    std::uint64_t id (0);
    assert (xuser::get_id (nullptr, user, &id) == S_OK && id == xuid ());

    std::uint64_t local (0);
    assert (xuser::get_local_id (nullptr, user, &local) == S_OK);
    assert (local == 1);

    user_state s (user_state::signed_out);
    assert (xuser::get_state (nullptr, user, &s) == S_OK);
    assert (s == user_state::signed_in);

    age_group g (age_group::unknown);
    assert (xuser::get_age_group (nullptr, user, &g) == S_OK);
    assert (g == age_group::adult);

    bool guest (true);
    assert (xuser::get_is_guest (nullptr, user, &guest) == S_OK && !guest);

    assert (xuser::compare (nullptr, user, user) == 0);
    assert (xuser::compare (nullptr, user, nullptr) != 0);

    bool        allowed (false);
    deny_reason why (deny_reason::banned);

    assert (xuser::check_privilege (nullptr, user, 0, 254, &allowed, &why) ==
            S_OK);
    assert (allowed && why == deny_reason::none);

    void* other (nullptr);

    assert (xuser::duplicate_handle (nullptr, user, &other) == S_OK);
    assert (other == user);

    assert (xuser::close_handle (nullptr, other) == S_OK);

    int nobody (0);
    assert (xuser::get_id (nullptr, &nobody, &id) == E_INVALIDARG);
    assert (xuser::duplicate_handle (nullptr, &nobody, &other) ==
            E_INVALIDARG);
  }

  {
    async_block b {};
    b.queue = queue;

    assert (xuser::resolve_privilege_async (nullptr, user, 0, 254, &b) ==
            S_OK);
    drain ();

    assert (xuser::resolve_privilege_result (nullptr, &b) == S_OK);
  }

  {
    auto f (reinterpret_cast<char (*) (void*, std::uint32_t)> (
              const_cast<void*> (slot_of<xruntime_feature> (0x18))));

    assert (f (self<xruntime_feature> (),
               static_cast<std::uint32_t> (feature::user)) == 1);

    assert (f (self<xruntime_feature> (),
               static_cast<std::uint32_t> (feature::display)) == 0);
  }

  {
    set_gamertag ("a-very-long-player-name-indeed");

    char        b[128];
    std::size_t n (0);

    assert (xuser_gamertag::get_gamertag (nullptr,
                                          user,
                                          gamertag_component::classic,
                                          sizeof (b),
                                          b,
                                          &n) == S_OK);

    assert (std::strcmp (b, "a-very-long-pla") == 0 && n == 16);

    assert (xuser_gamertag::get_gamertag (nullptr,
                                          user,
                                          gamertag_component::modern_suffix,
                                          sizeof (b),
                                          b,
                                          &n) == S_OK);

    assert (b[0] == '\0' && n == 1);

    assert (xuser_gamertag::get_gamertag (
              nullptr,
              user,
              static_cast<gamertag_component> (9),
              sizeof (b),
              b,
              &n) == E_INVALIDARG);
  }

  {
    async_block b {};
    b.queue = queue;

    assert (xuser::get_token_and_signature_async (nullptr,
                                                  user,
                                                  0,
                                                  "GET",
                                                  "https://xboxlive.com/x",
                                                  0, nullptr, 0, nullptr,
                                                  &b) == E_NOTIMPL);

    serve (&ours);

    async_block t {};
    t.queue = queue;

    assert (xuser::get_token_and_signature_async (nullptr,
                                                  user,
                                                  0,
                                                  "GET",
                                                  "http://iw4x/x",
                                                  0, nullptr, 0, nullptr,
                                                  &t) == S_OK);
    drain ();

    std::size_t n (0);
    assert (xuser::get_token_and_signature_result_size (nullptr, &t, &n) ==
            S_OK);
    assert (n > sizeof (token_and_signature));

    char        buffer[512];
    void*       out (nullptr);
    std::size_t used (0);

    assert (xuser::get_token_and_signature_result (nullptr,
                                                   &t,
                                                   sizeof (buffer),
                                                   buffer,
                                                   &out,
                                                   &used) == S_OK);

    auto* d (static_cast<token_and_signature*> (out));

    assert (std::strncmp (d->token, "IW4x1.0 xuid=", 13) == 0);
    assert (d->token_size == std::strlen (d->token));
    assert (d->signature_size == 0);

    serve (nullptr);
  }

  {
    std::uint64_t token (0);

    assert (xuser::register_for_change (
              nullptr,
              nullptr,
              nullptr,
              [] (void*, std::uint64_t, std::uint32_t) {},
              &token) == S_OK);

    assert (token != 0);
    assert (xuser::unregister_for_change (nullptr, token, false) == S_OK);
    assert (xuser::unregister_for_change (nullptr, token, false) ==
            E_INVALIDARG);
  }

  {
    pick_players_with (&first_two);

    const std::uint64_t candidates[] {10, 20, 30};

    async_block b {};
    b.queue = queue;

    assert (xgame_ui::show_player_picker_async (nullptr,
                                                &b,
                                                user,
                                                "Choose",
                                                3,
                                                candidates,
                                                0, nullptr, 0,
                                                3) == S_OK);
    drain ();

    std::uint64_t chosen[8] {};
    std::uint32_t count (0);

    assert (xgame_ui::show_player_picker_result (nullptr,
                                                 &b,
                                                 8,
                                                 chosen,
                                                 &count) == S_OK);

    assert (count == 2 && chosen[0] == 10 && chosen[1] == 20);

    pick_players_with (nullptr);
  }

  {
    activation u (activation_uri ("a b&c=d"));

    assert (std::strcmp (u.c_str (),
                         "ms-xbl-multiplayer://activity?connectionString="
                         "a%20b%26c%3Dd") == 0);

    std::uint64_t token (0);

    assert (xgame_invite::register_for_event (
              nullptr,
              nullptr,
              nullptr,
              [] (void*, const char* uri)
              {
                assert (std::strstr (uri, "connectionString=hello") !=
                        nullptr);
              },
              &token) == S_OK);

    deliver_invite ("hello");
    drain ();

    assert (xgame_invite::unregister_for_event (nullptr, token, false) == 1);
    assert (xgame_invite::unregister_for_event (nullptr, token, false) == 0);
  }

  {
    async_block b {};
    b.queue = queue;

    assert (xnetworking::query_security_information_async (nullptr,
                                                           nullptr,
                                                           &b) == S_OK);
    drain ();

    std::size_t n (1);

    assert (xnetworking::query_security_information_result_size (nullptr,
                                                                 &b,
                                                                 &n) == S_OK);
    assert (n == 0);

    std::size_t used (1);
    void*       out (&used);

    assert (xnetworking::query_security_information_result (nullptr,
                                                            &b,
                                                            0,
                                                            &used,
                                                            nullptr,
                                                            &out) == S_OK);
    assert (out == nullptr && used == 0);

    assert (xnetworking::verify_server_certificate (nullptr,
                                                    nullptr,
                                                    nullptr) == S_OK);

    assert (xnetworking::verify_server_certificate (nullptr,
                                                    nullptr,
                                                    &used) == S_OK);
  }

  {
    connectivity_hint h {};

    assert (xnetworking::get_connectivity_hint (nullptr, &h) == S_OK);
    assert (h.network_initialized);
    assert (h.level == 3);

    std::uint64_t token (0);

    assert (xnetworking::register_connectivity_changed (
              nullptr,
              nullptr,
              nullptr,
              [] (void*, const connectivity_hint* x)
              {
                assert (x->network_initialized);
              },
              &token) == S_OK);

    drain ();

    assert (xnetworking::unregister_connectivity_changed (nullptr,
                                                          token,
                                                          false) == S_OK);
  }

  {
    assert (storable ("settings"));
    assert (storable ("a.b"));
    assert (storable (".hidden"));

    assert (!storable (""));
    assert (!storable ("."));
    assert (!storable (".."));
    assert (!storable ("a/b"));
    assert (!storable ("a\\b"));
    assert (!storable ("c:"));
    assert (!storable ("a*"));
    assert (!storable ("a?"));
    assert (!storable ("a\""));
    assert (!storable ("a<"));
    assert (!storable ("a>"));
    assert (!storable ("a|"));
  }

  {
    async_block b {};
    b.queue = queue;

    assert (xgame_save::initialize_provider_async (nullptr,
                                                   user,
                                                   "scid",
                                                   false,
                                                   &b) == S_OK);
    drain ();

    save_provider* p (nullptr);
    assert (xgame_save::initialize_provider_result (nullptr, &b, &p) == S_OK);
    assert (p != nullptr);

    save_container* c (nullptr);
    assert (xgame_save::create_container (nullptr, p, "settings", &c) ==
            S_OK);
    assert (c != nullptr);

    assert (xgame_save::create_container (nullptr, p, "..", &c) ==
            E_INVALIDARG);
    assert (xgame_save::create_container (nullptr, p, "a/b", &c) ==
            E_INVALIDARG);

    save_update* u (nullptr);
    assert (xgame_save::create_update (nullptr, c, "display", &u) == S_OK);

    const std::uint8_t one[] {1, 2, 3, 4};
    const std::uint8_t two[] {9, 8, 7, 6, 5};

    assert (xgame_save::submit_blob_write (nullptr, u, "one", one, 4) ==
            S_OK);
    assert (xgame_save::submit_blob_write (nullptr, u, "two", two, 5) ==
            S_OK);

    async_block s {};
    s.queue = queue;

    assert (xgame_save::submit_update_async (nullptr, u, &s) == S_OK);
    drain ();

    assert (xgame_save::submit_update (nullptr, &s) == S_OK);

    xgame_save::close_update (nullptr, u);

    const char* names[] {"one", "two"};

    async_block r {};
    r.queue = queue;

    assert (xgame_save::read_blob_data_async (nullptr, c, names, 2, &r) ==
            S_OK);
    drain ();

    std::size_t n (0);
    assert (result_size (&r, &n) == S_OK);

    char          buffer[512];
    std::uint32_t got (0);

    assert (xgame_save::read_blob_data (nullptr,
                                        &r,
                                        sizeof (buffer),
                                        buffer,
                                        &got) == S_OK);

    assert (got == 2);

    auto* bs (reinterpret_cast<save_blob*> (buffer));

    assert (std::strcmp (bs[0].info.name, "one") == 0);
    assert (bs[0].info.size == 4 && bs[0].data[0] == 1 && bs[0].data[3] == 4);

    assert (std::strcmp (bs[1].info.name, "two") == 0);
    assert (bs[1].info.size == 5 && bs[1].data[4] == 5);

    async_block h {};
    h.queue = queue;

    assert (xgame_save::read_blob_data_async (nullptr, c, names, 2, &h) ==
            S_OK);
    drain ();

    save_blob     headers[3] {};
    std::uint32_t none (7);

    assert (xgame_save::read_blob_data (nullptr,
                                        &h,
                                        sizeof (save_blob),
                                        headers,
                                        &none) == insufficient_buffer);

    assert (none == 7);
    assert (headers[1].info.name == nullptr && headers[1].data == nullptr);
    assert (headers[2].info.name == nullptr && headers[2].data == nullptr);

    const char* missing[] {"missing"};

    async_block m {};
    m.queue = queue;

    assert (xgame_save::read_blob_data_async (nullptr, c, missing, 1, &m) ==
            S_OK);
    drain ();

    assert (result_size (&m, &n) == blob_not_found);

    enumerated = 0;

    assert (xgame_save::enumerate_container_info (nullptr,
                                                  p,
                                                  nullptr,
                                                  &count_container) == S_OK);
    assert (enumerated == 1);

    xgame_save::close_container (nullptr, c);
    xgame_save::close_provider (nullptr, p);
  }

  assert (xuser::close_handle (nullptr, user) == S_OK);

  assert (UninitializeApiImpl () == S_OK);
  assert (!initialized ());
}
