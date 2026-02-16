#include <libiw4x/types.hxx>
#include <libiw4x/scheduler.hxx>

#undef NDEBUG
#include <cassert>

int
main ()
{
  using namespace std;
  using namespace iw4x;

  scheduler s;

  // Test strand creation.
  //
  assert (s.create ("strand1"));
  assert (s.exists ("strand1"));
  assert (!s.create ("strand1")); // Already exists.
  assert (s.create ("strand2"));
  assert (s.exists ("strand2"));

  // Test posting work to strands.
  //
  {
    bool executed (false);

    assert (s.post ("strand1", [&executed] () { executed = true; }));
    assert (!executed); // Not yet polled.

    s.poll ("strand1");

    assert (executed);
  }

  // Test multiple work items on same strand.
  //
  {
    int counter (0);

    assert (s.post ("strand2", [&counter] () { counter++; }));
    assert (s.post ("strand2", [&counter] () { counter++; }));
    assert (s.post ("strand2", [&counter] () { counter++; }));

    assert (counter == 0);

    s.poll ("strand2");

    assert (counter == 3);
  }

  // Test posting to non-existent strand.
  //
  {
    bool executed (false);

    assert (!s.post ("nonexistent", [&executed] () { executed = true; }));
    assert (!executed);
  }

  // Test work ordering on same strand.
  //
  {
    string result;

    assert (s.post ("strand1", [&result] () { result += "a"; }));
    assert (s.post ("strand1", [&result] () { result += "b"; }));
    assert (s.post ("strand1", [&result] () { result += "c"; }));

    s.poll ("strand1");

    assert (result == "abc");
    assert (s.post ("strand1", [&result] () { result += "d"; }));

    s.poll ("strand1");

    assert (result == "abcd");
  }
}
