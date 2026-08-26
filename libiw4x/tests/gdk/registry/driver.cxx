#undef NDEBUG
#include <cassert>

#include <cstring>

#include <libiw4x/gdk/types.hxx>
#include <libiw4x/gdk/vtable.hxx>
#include <libiw4x/gdk/async.hxx>
#include <libiw4x/gdk/system.hxx>
#include <libiw4x/gdk/feature.hxx>
#include <libiw4x/gdk/registry.hxx>

using namespace iw4x::gdk;

int
main ()
{
  {
    assert (xsystem::api == xsystem::api);
    assert (!(xsystem::api == xsystem::id));
    assert (xruntime_feature::api == xruntime_feature::id);
  }

  {
    assert (slot_of<xsystem> (0x20) ==
            reinterpret_cast<const void*> (&xsystem::get_xbox_live_sandbox_id));

    assert (slot_of<xsystem> (0x00) ==
            reinterpret_cast<const void*> (&query_interface));

    assert (slot_of<xsystem> (0x18) != nullptr);
    assert (slot_of<xsystem> (0x18) != slot_of<xsystem> (0x28));

    assert (slot_of<xsystem> (max_slot) == nullptr);
    assert (slot_of<xsystem> (0x21) == nullptr);
  }

  {
    assert (object_of<xsystem>.table == table_of<xsystem>::entries);

    const void* const* first;

    std::memcpy (&first, &object_of<xsystem>, sizeof (first));

    assert (first == table_of<xsystem>::entries);
  }

  {
    assert (find_interface (xsystem::api, xsystem::id) == &object_of<xsystem>);

    assert (find_interface (xruntime_feature::api, xruntime_feature::id) ==
            &object_of<xruntime_feature>);

    assert (find_interface (xsystem::api, xruntime_feature::id) == nullptr);
    assert (find_interface (xruntime_feature::api, xsystem::id) == nullptr);
  }

  {
    assert (provides (feature::system));
    assert (provides (feature::async));
    assert (provides (feature::task_queue));
    assert (provides (feature::error));
    assert (provides (feature::game_save));
    assert (provides (feature::user));
    assert (provides (feature::store));
    assert (provides (feature::game_ui));
    assert (provides (feature::game_invite));
    assert (provides (feature::networking));

    assert (!provides (feature::display));
    assert (!provides (feature::app_capture));
    assert (!provides (feature::package));
    assert (!provides (feature::game_event));
    assert (!provides (static_cast<feature> (99)));

    assert (interface_count () == 13);
    assert (family_count () == 11);
  }

  {
    auto f (reinterpret_cast<char (*) (void*, unsigned)> (
              const_cast<void*> (slot_of<xruntime_feature> (0x18))));

    void* self (const_cast<interface_object*> (&object_of<xruntime_feature>));

    assert (f (self, static_cast<unsigned> (feature::system)) == 1);
    assert (f (self, static_cast<unsigned> (feature::display)) == 0);
    assert (f (self, 99) == 0);
  }

  {
    char        b[16];
    std::size_t n (0);

    auto f (reinterpret_cast<long (*) (void*, std::size_t, char*, std::size_t*)> (
              const_cast<void*> (slot_of<xsystem> (0x20))));

    void* self (const_cast<interface_object*> (&object_of<xsystem>));

    assert (f (self, sizeof (b), b, &n) == S_OK);
    assert (std::strcmp (b, "RETAIL") == 0);
    assert (n == 7);

    assert (f (self, 3, b, &n) == insufficient_buffer);
    assert (f (self, sizeof (b), nullptr, &n) == E_POINTER);
  }

  {
    auto f (reinterpret_cast<long (*) (void*, void*, void*, void*, void*)> (
              const_cast<void*> (slot_of<xsystem> (0x28))));

    void* self (const_cast<interface_object*> (&object_of<xsystem>));

    assert (f (self, nullptr, nullptr, nullptr, nullptr) == E_NOTIMPL);
  }
}
