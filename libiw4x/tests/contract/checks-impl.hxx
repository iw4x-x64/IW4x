// Contract checks for a single checking level.
//
// This file is included by each of the checks-*.cxx translation units,
// which select the level being tested by defining LIBIW4X_CONTRACT
// and name the resulting level object with TEST_LEVEL_ID and
// TEST_LEVEL_NAME. It is therefore deliberately without an include
// guard.
//
#if !defined(LIBIW4X_CONTRACT) ||    \
    !defined(TEST_LEVEL_ID)       || \
    !defined(TEST_LEVEL_NAME)
#  error this file must only be included by the checks-*.cxx translation units
#endif

#include <exception> // exception_ptr, current_exception(),
                     // rethrow_exception()

#include <libiw4x/contract.hxx>

#include "checks.hxx"

namespace test
{
  // Note that all three translation units get their own copy of these
  // functions. Apart from avoiding duplicate definitions, this lets
  // each copy be compiled with its own LIBIW4X_CONTRACT value.
  //
  namespace
  {
    // There is a somewhat odd-looking detail common to the checks
    // below: the assignment to l and the contract check are kept on the
    // same physical source line.
    //
    // The failure record contains the line at which the contract macro
    // was expanded. We want to compare that line with an independently
    // recorded value in the driver, so __LINE__ has to observe exactly
    // the same line as the macro. Splitting these two statements over
    // separate lines would make every such test off by one without
    // changing anything interesting about the contract machinery
    // itself.
    //
    // Note that preconditions and the audit checks can disappear
    // completely at some checking levels. Their ok parameters are
    // marked maybe_unused for this reason. Assertions are different: at
    // the off level they become optimizer assumptions and still consume
    // the expression.
    //
    void
    pre ([[maybe_unused]] bool ok, unsigned& l)
    {
      l = __LINE__; LIBIW4X_PRE (ok);
    }

    void
    pre_msg ([[maybe_unused]] bool ok, unsigned& l)
    {
      l = __LINE__; LIBIW4X_PRE_MSG (ok, "must hold");
    }

    void
    assertion (bool ok, unsigned& l)
    {
      l = __LINE__; LIBIW4X_ASSERT (ok);
    }

    void
    assertion_msg (bool ok, unsigned& l)
    {
      l = __LINE__; LIBIW4X_ASSERT_MSG (ok, "must hold");
    }

    void
    unreachable (bool, unsigned& l)
    {
      l = __LINE__; LIBIW4X_UNREACHABLE ();
    }

    void
    post ([[maybe_unused]] bool ok, unsigned& l)
    {
      l = __LINE__; LIBIW4X_POST (ok);
    }

    void
    post_final (bool ok, unsigned& l)
    {
      [[maybe_unused]] bool v (false);

      l = __LINE__; LIBIW4X_POST (v);

      v = ok;
    }

    void
    post_unwind ([[maybe_unused]] bool ok, unsigned& l)
    {
      l = __LINE__; LIBIW4X_POST (ok);

      throw unwind ();
    }

    void
    unwinding_check ([[maybe_unused]] bool ok, unsigned& l)
    {
      l = __LINE__; LIBIW4X_POST (ok);
    }

    void
    post_unwinding (bool ok, unsigned& l)
    {
      std::exception_ptr e;

      struct scope
      {
        bool                ok;
        unsigned&           l;
        std::exception_ptr& e;

        ~scope ()
        {
          // At this point the unwind exception from below is already
          // active. Entering unwinding_check() now gives its
          // postcondition guard that existing exception count as the
          // starting point.
          //
          try
          {
            unwinding_check (ok, l);
          }
          catch (...)
          {
            e = std::current_exception ();
          }
        }
      };

      try
      {
        scope s {ok, l, e};
        throw unwind ();
      }
      catch (const unwind&) {}

      if (e != nullptr)
        std::rethrow_exception (e);
    }

    void
    invariant_entry ([[maybe_unused]] bool ok, unsigned& l)
    {
      l = __LINE__; LIBIW4X_INVARIANT (ok);
    }

    void
    invariant_exit (bool ok, unsigned& l)
    {
      [[maybe_unused]] bool v (true);

      l = __LINE__; LIBIW4X_INVARIANT (v);

      v = ok;
    }

    void
    invariant_unwind (bool ok, unsigned& l)
    {
      [[maybe_unused]] bool v (true);

      l = __LINE__; LIBIW4X_INVARIANT (v);

      v = ok;

      throw unwind ();
    }

    const check checks[] =
    {
      // name              function           assume  always_fails
      //
      {"pre",              &pre,              false,  false},
      {"pre-msg",          &pre_msg,          false,  false},
      {"assertion",        &assertion,        true,   false},
      {"assertion-msg",    &assertion_msg,    true,   false},
      {"unreachable",      &unreachable,      true,   true },
      {"post",             &post,             false,  false},
      {"post-final",       &post_final,       false,  false},
      {"post-unwind",      &post_unwind,      false,  false},
      {"post-unwinding",   &post_unwinding,   false,  false},
      {"invariant-entry",  &invariant_entry,  false,  false},
      {"invariant-exit",   &invariant_exit,   false,  false},
      {"invariant-unwind", &invariant_unwind, false,  false},
      {nullptr,            nullptr,           false,  false}
    };
  }

  // The declaration in checks.hxx has already declared this object
  // extern, which gives this definition the linkage needed by the
  // common driver.
  //
  // Each including translation unit substitutes its own name and
  // checking level here, paired with the private checks table compiled
  // for that level.
  //
  const level TEST_LEVEL_ID {TEST_LEVEL_NAME,
                             LIBIW4X_CONTRACT,
                             __FILE__,
                             checks};
}
