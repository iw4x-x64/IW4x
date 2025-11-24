#include <stdexcept>
#include <string>

#include <libiw4x/utility/scheduler.hxx>

#undef NDEBUG
#include <cassert>

int main ()
{
  using namespace std;
  using namespace iw4x::utility;

  // Test register_strand
  //
  {
    scheduler sched;
    sched.register_strand ("test_strand");

    // Verify strand is registered
    //
    assert (sched.is_registered ("test_strand"));

    // Verify different name is not registered
    //
    assert (!sched.is_registered ("other_strand"));
  }

  // Test register_strand with empty name
  //
  {
    scheduler sched;

    try
    {
      sched.register_strand ("");
      assert (false);
    }
    catch (const invalid_argument& e)
    {
      assert (e.what () == string ("strand name cannot be empty"));
    }
  }

  // Test register_strand with duplicate name
  //
  {
    scheduler sched;
    sched.register_strand ("duplicate");

    try
    {
      sched.register_strand ("duplicate");
      assert (false);
    }
    catch (const invalid_argument& e)
    {
      assert (e.what () == string ("strand name already registered: duplicate"));
    }
  }

  // Test unregister_strand
  //
  {
    scheduler sched;
    sched.register_strand ("temp");
    assert (sched.is_registered ("temp"));

    sched.unregister_strand ("temp");
    assert (!sched.is_registered ("temp"));
  }

  // Test unregister_strand with empty name
  //
  {
    scheduler sched;

    try
    {
      sched.unregister_strand ("");
      assert (false);
    }
    catch (const invalid_argument& e)
    {
      assert (e.what () == string ("strand name cannot be empty"));
    }
  }

  // Test unregister_strand with non-existent strand
  //
  {
    scheduler sched;

    try
    {
      sched.unregister_strand ("nonexistent");
      assert (false);
    }
    catch (const invalid_argument& e)
    {
      assert (e.what () == string ("strand not registered: nonexistent"));
    }
  }

  // Test post and poll with single task
  //
  {
    scheduler sched;
    sched.register_strand ("main");

    int counter (0);

    sched.post ("main", [&counter] () { ++counter; });

    // Task should not execute until poll
    //
    assert (counter == 0);
    assert (!sched.get_io_context("main").stopped());

    sched.poll ("main");

    // Task should have executed
    //
    assert (counter == 1);
    assert (sched.get_io_context("main").stopped());
  }

  // Test post and poll with multiple tasks
  //
  {
    scheduler sched;
    sched.register_strand ("main");

    int sum (0);

    sched.post ("main", [&sum] () { sum += 1; });
    sched.post ("main", [&sum] () { sum += 2; });
    sched.post ("main", [&sum] () { sum += 3; });

    assert (!sched.get_io_context("main").stopped());

    sched.poll ("main");

    // All tasks should have executed
    //
    assert (sum == 6);
    assert (sched.get_io_context("main").stopped());
  }

  // Test post to unregistered strand
  //
  {
    scheduler sched;

    try
    {
      sched.post ("missing", [] () {});
      assert (false);
    }
    catch (const invalid_argument& e)
    {
      assert (e.what () == string ("strand not registered: missing"));
    }
  }

  // Test poll on unregistered strand
  //
  {
    scheduler sched;

    try
    {
      sched.poll ("missing");
      assert (false);
    }
    catch (const invalid_argument& e)
    {
      assert (e.what () == string ("strand not registered: missing"));
    }
  }

  // Test has_pending on unregistered strand
  //
  {
    scheduler sched;

    try
    {
      sched.get_io_context("missing");
      assert (false);
    }
    catch (const invalid_argument& e)
    {
      assert (e.what () == string ("strand not registered: missing"));
    }
  }

  // Test poll with no pending tasks
  //
  {
    scheduler sched;
    sched.register_strand ("empty");

    assert (!sched.get_io_context("empty").stopped());

    // Should not throw, just do nothing
    //
    sched.poll ("empty");

    assert (sched.get_io_context("empty").stopped());
  }

  // Test multiple strands
  //
  {
    scheduler sched;
    sched.register_strand ("strand_a");
    sched.register_strand ("strand_b");

    int counter_a (0);
    int counter_b (0);

    sched.post ("strand_a", [&counter_a] () { ++counter_a; });
    sched.post ("strand_b", [&counter_b] () { ++counter_b; });

    // Both should have pending tasks
    //
    assert (!sched.get_io_context("strand_a").stopped());
    assert (!sched.get_io_context("strand_b").stopped());

    // Poll only strand_a
    //
    sched.poll ("strand_a");

    assert (counter_a == 1);
    assert (counter_b == 0);
    assert (sched.get_io_context("strand_a").stopped());
    assert (!sched.get_io_context("strand_b").stopped());

    // Poll strand_b
    //
    sched.poll ("strand_b");

    assert (counter_a == 1);
    assert (counter_b == 1);
    assert (sched.get_io_context("strand_b").stopped());
  }

  // Test task execution order (FIFO)
  //
  {
    scheduler sched;
    sched.register_strand ("ordered");

    string result;

    sched.post ("ordered", [&result] () { result += "A"; });
    sched.post ("ordered", [&result] () { result += "B"; });
    sched.post ("ordered", [&result] () { result += "C"; });

    sched.poll ("ordered");

    assert (result == "ABC");
  }

  // Test unregister clears pending tasks
  //
  {
    scheduler sched;
    sched.register_strand ("temp");

    int counter (0);
    sched.post ("temp", [&counter] () { ++counter; });

    assert (!sched.get_io_context("temp").stopped());

    sched.unregister_strand ("temp");

    // strand is gone, counter should not have changed
    //
    assert (counter == 0);
    assert (!sched.is_registered ("temp"));
  }

  // Test re-registering after unregister
  //
  {
    scheduler sched;
    sched.register_strand ("reuse");

    int counter (0);
    sched.post ("reuse", [&counter] () { ++counter; });

    sched.unregister_strand ("reuse");
    sched.register_strand ("reuse");

    // New strand should have no pending tasks
    //
    assert (!sched.get_io_context("reuse").stopped());

    sched.post ("reuse", [&counter] () { ++counter; });
    sched.poll ("reuse");

    // Only the second task should have executed
    //
    assert (counter == 1);
  }

  // Test post with capturing lambda
  //
  {
    scheduler sched;
    sched.register_strand ("capture");

    int value (42);
    bool executed (false);

    sched.post ("capture", [&executed, value] ()
    {
      executed = true;
      assert (value == 42);
    });

    sched.poll ("capture");
    assert (executed);
  }

  // Test post with mutable lambda
  //
  {
    scheduler sched;
    sched.register_strand ("mutable");

    int external (0);

    sched.post ("mutable", [&external] () mutable
    {
      external = 100;
    });

    sched.poll ("mutable");
    assert (external == 100);
  }

  // Test multiple polls on same strand
  //
  {
    scheduler sched;
    sched.register_strand ("multi_poll");

    int counter (0);

    sched.post ("multi_poll", [&counter] () { ++counter; });
    sched.poll ("multi_poll");
    assert (counter == 1);

    sched.post ("multi_poll", [&counter] () { ++counter; });
    sched.poll ("multi_poll");
    assert (counter == 2);

    sched.post ("multi_poll", [&counter] () { ++counter; });
    sched.poll ("multi_poll");
    assert (counter == 3);
  }

  // Test post after poll
  //
  {
    scheduler sched;
    sched.register_strand ("continuous");

    int counter (0);

    sched.post ("continuous", [&counter] () { ++counter; });
    sched.poll ("continuous");

    assert (counter == 1);
    assert (sched.get_io_context("continuous").stopped());

    sched.post ("continuous", [&counter] () { ++counter; });

    assert (sched.get_io_context("continuous").stopped());

    sched.poll ("continuous");

    assert (counter == 2);
  }

  // Test task that posts another task
  //
  {
    scheduler sched;
    sched.register_strand ("recursive");

    int counter (0);

    sched.post ("recursive", [&sched, &counter] ()
    {
      ++counter;
      sched.post ("recursive", [&counter] () { ++counter; });
    });

    sched.poll ("recursive");

    // All task executed
    //
    assert (counter == 2);
    assert (sched.get_io_context("recursive").stopped());
  }

  // Test strand names are case-sensitive
  //
  {
    scheduler sched;
    sched.register_strand ("CaseSensitive");
    sched.register_strand ("casesensitive");

    assert (sched.is_registered ("CaseSensitive"));
    assert (sched.is_registered ("casesensitive"));

    // These are different strands
    //
    int counter_upper (0);
    int counter_lower (0);

    sched.post ("CaseSensitive", [&counter_upper] () { ++counter_upper; });
    sched.post ("casesensitive", [&counter_lower] () { ++counter_lower; });

    sched.poll ("CaseSensitive");
    assert (counter_upper == 1);
    assert (counter_lower == 0);

    sched.poll ("casesensitive");
    assert (counter_upper == 1);
    assert (counter_lower == 1);
  }

  // Test strand with special characters
  //
  {
    scheduler sched;
    sched.register_strand ("pipe-line_123!@#");

    assert (sched.is_registered ("pipe-line_123!@#"));

    int counter (0);
    sched.post ("pipe-line_123!@#", [&counter] () { ++counter; });
    sched.poll ("pipe-line_123!@#");

    assert (counter == 1);
  }

  // Test has_pending returns false after poll
  //
  {
    scheduler sched;
    sched.register_strand ("check_pending");

    sched.post ("check_pending", [] () {});

    assert (!sched.get_io_context("check_pending").stopped ());

    sched.poll ("check_pending");

    assert (sched.get_io_context("check_pending").stopped ());
  }

  // Test exception safety in tasks
  //
  {
    scheduler sched;
    sched.register_strand ("exception_test");

    int counter (0);

    sched.post ("exception_test", [&counter] () { ++counter; });
    sched.post ("exception_test", [] () { throw runtime_error ("test exception"); });
    sched.post ("exception_test", [&counter] () { ++counter; });

    // Poll should execute first task, throw on second, and not reach third
    //
    try
    {
      sched.poll ("exception_test");
      assert (false);
    }
    catch (const runtime_error& e)
    {
      assert (e.what () == string ("test exception"));
    }

    // First task executed, third task never reached
    //
    assert (counter == 1);
  }

  // Test long strand names
  //
  {
    scheduler sched;
    string long_name (1000, 'x');
    sched.register_strand (long_name);

    assert (sched.is_registered (long_name));

    int counter (0);
    sched.post (long_name, [&counter] () { ++counter; });
    sched.poll (long_name);

    assert (counter == 1);
  }

  // Test destructor clears all strands
  //
  {
    int counter (0);

    {
      scheduler sched;
      sched.register_strand ("temp");
      sched.post ("temp", [&counter] () { ++counter; });

      // Scheduler destroyed here, tasks not executed
    }

    assert (counter == 0);
  }
}
