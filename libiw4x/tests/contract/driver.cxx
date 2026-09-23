#include <cerrno>   // errno, ERANGE
#include <climits>  // UINT_MAX
#include <csignal>  // signal(), SIGABRT, SIG_ERR
#include <cstdio>   // fflush(), stdout
#include <cstdlib>  // strtoul(), _Exit()
#include <cstring>  // strcmp()
#include <ios>      // ios::*bit
#include <string>
#include <iostream>
#include <type_traits> // is_copy_*

// Note that this is also what defines __cpp_lib_jthread, which is what
// we test for below.
//
#ifdef __STDCPP_THREADS__
#  include <thread> // jthread
#endif

#include <libiw4x/contract.hxx>

#include "checks.hxx"

#undef NDEBUG
#include <cassert>

using namespace std;
using namespace iw4x;

// Test driver for contract checking.
//
// Usages:
//
// argv[0] kinds
// argv[0] levels
// argv[0] handler
// argv[0] thread
// argv[0] fail        <kind> <file> <line> <function> <expr> [<message>]
// argv[0] report [-r] <kind> <file> <line> <function> <expr> [<message>]
// argv[0] check <level> <check> (true|false)
//
// In the kinds form print the name of each contract kind, one per line,
// followed by the name of a kind that is not part of the enumeration.
//
// In the levels form print the LIBIW4X_CONTRACT value each of the
// checks-*.cxx translation units is built at, one <level> <value> pair
// per line. The last line is for this translation unit, which does not
// define LIBIW4X_CONTRACT at all.
//
// In the handler form install, replace, and restore the contract
// handler, printing the previously installed handler each time.
//
// In the thread form report a violation from another thread using the
// handler installed in this one and print the resulting violation.
//
// In the fail form report a violation by calling contract_fail() with
// the specified arguments and print the violation as it is seen by the
// handler. The <file>, <function>, <expr>, and <message> arguments can
// be spelled as <null> to pass NULL.
//
// In the report form do the same but without installing a handler,
// which makes the default reporting print the violation to stderr and
// terminate the process (see below). If -r is specified, then install a
// handler that prints the handled string to stdout and returns instead
// of throwing.
//
// In the check form perform the specified contract check at the
// specified checking level with the specified outcome and print the
// resulting violation, if any. If the check left its scope via an
// exception instead, then print the unwound string.
//
// A violation is printed in one of the following forms, with a NULL
// field printed as <null>:
//
//   <kind>|<file>|<line>|<function>|<expression>|<message>
//   <kind>|<function>|<expression>|<message>
//
// The second (locationless) form is used for the check form: there the
// location is not supplied by the caller but comes from the macro being
// tested, so the driver verifies it against the recorded location of
// the check instead of printing a line number that the testscript would
// have to keep in sync.
//
namespace
{
  // The contract kinds, in the enumeration order.
  //
  const contract_kind kinds[] =
  {
    contract_kind::precondition,
    contract_kind::postcondition,
    contract_kind::invariant,
    contract_kind::assertion,
    contract_kind::unreachable
  };

  // Contract checking terminates the process by calling abort(). To be
  // able to test the default reporting we catch the resulting SIGABRT
  // and exit with the following status instead.
  //
  const int abort_status = 42;

  extern "C" void
  abort_handler (int)
  {
    // Note that the signal is raised synchronously by abort() in a
    // single-threaded process. Still, keep this to the
    // async-signal-safe minimum: everything that is printed on this
    // path is flushed before the violation is reported.
    //
    std::_Exit (abort_status);
  }

  void
  trap_abort ()
  {
#ifdef _MSC_VER
    // Suppress the abort message and the crash dialog that the
    // Microsoft runtime would otherwise print/show in addition to
    // raising the signal. Note that this is not available in the MinGW
    // default runtime (msvcrt), which does not print the message if the
    // signal is handled.
    //
    _set_abort_behavior (0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif

    assert (signal (SIGABRT, &abort_handler) != SIG_ERR);
  }

  // Number of times a contract handler has been called.
  //
  size_t handler_calls (0);

  // Thrown by the handler below to report a violation without
  // terminating the process.
  //
  struct violation
  {
    contract_violation v;
  };

  void
  throwing_handler (const contract_violation& v)
  {
    ++handler_calls;
    throw violation {v};
  }

  // A handler that returns, which must not discharge the violation.
  //
  void
  returning_handler (const contract_violation&)
  {
    ++handler_calls;

    // The process is about to be terminated with _Exit(), which does
    // not flush the standard streams.
    //
    cout << "handled" << endl;
    fflush (stdout);
  }

  void
  print (const contract_violation& v, bool location)
  {
    auto s = [] (const char* p) {return p != nullptr ? p : "<null>";};

    cout << contract_kind_name (v.kind) << '|';

    if (location)
      cout << s (v.file) << '|' << v.line << '|';

    cout << s (v.function) << '|' << s (v.expression) << '|' << s (v.message)
         << endl;
  }

  // Note: mirrors the file name logic in contract.cxx.
  //
  const char*
  base_name (const char* p)
  {
    const char* r (p);

    for (const char* i (p); *i != '\0'; ++i)
    {
      if (*i == '/' || *i == '\\')
        r = i + 1;
    }

    return r;
  }

  // Command line argument parsing.
  //
  const char*
  text (const char* s)
  {
    return strcmp (s, "<null>") == 0 ? nullptr : s;
  }

  unsigned
  number (const char* s)
  {
    char* e (nullptr);
    errno = 0; // We must clear it according to POSIX.
    unsigned long r (strtoul (s, &e, 10));

    assert (errno != ERANGE && e != s && *e == '\0' && r <= UINT_MAX);

    return static_cast<unsigned> (r);
  }

  bool
  boolean (const char* s)
  {
    assert (strcmp (s, "true") == 0 || strcmp (s, "false") == 0);
    return strcmp (s, "true") == 0;
  }

  contract_kind
  kind (const char* s)
  {
    for (contract_kind k: kinds)
    {
      if (strcmp (contract_kind_name (k), s) == 0)
        return k;
    }

    assert (false); // Invalid contract kind name.
    return contract_kind::assertion;
  }

  const test::level*
  find_level (const char* s)
  {
    for (const test::level* l: {&test::level_off,
                                &test::level_default,
                                &test::level_audit})
    {
      if (strcmp (l->name, s) == 0)
        return l;
    }

    return nullptr;
  }

  const test::check*
  find_check (const test::level& l, const char* s)
  {
    for (const test::check* c (l.checks); c->name != nullptr; ++c)
    {
      if (strcmp (c->name, s) == 0)
        return c;
    }

    return nullptr;
  }
}

// Compile-time properties of the interface.
//
namespace
{
  struct bool_predicate {bool operator() () const;};
  struct int_predicate  {int  operator() () const;};
  struct void_predicate {void operator() () const;};
}

static_assert (LIBIW4X_CONTRACT_OFF     == 0);
static_assert (LIBIW4X_CONTRACT_DEFAULT == 1);
static_assert (LIBIW4X_CONTRACT_AUDIT   == 2);

static_assert (contract_predicate<bool_predicate>);
static_assert (contract_predicate<int_predicate>);
static_assert (!contract_predicate<void_predicate>);

// A guard must not be copied: the copy would check the same contract
// again when it is destroyed.
//
static_assert (!is_copy_constructible_v<postcondition_guard<bool_predicate>>);
static_assert (!is_copy_assignable_v<postcondition_guard<bool_predicate>>);
static_assert (!is_copy_constructible_v<invariant_guard<bool_predicate>>);
static_assert (!is_copy_assignable_v<invariant_guard<bool_predicate>>);

namespace
{
  int
  print_kinds ()
  {
    for (contract_kind k: kinds)
      cout << contract_kind_name (k) << endl;

    // A kind that is not part of the enumeration. Note that the
    // enumeration has a fixed underlying type, so every value of that
    // type is a valid enumeration value.
    //
    cout << contract_kind_name (static_cast<contract_kind> (200)) << endl;

    return 0;
  }

  int
  print_levels ()
  {
    for (const test::level* l: {&test::level_off,
                                &test::level_default,
                                &test::level_audit})
      cout << l->name << ' ' << l->value << endl;

    // The level a translation unit that does not specify one is built
    // at (this one).
    //
    cout << "undefined " << LIBIW4X_CONTRACT << endl;

    return 0;
  }

  void first  (const contract_violation&) {}
  void second (const contract_violation&) {}

  const char*
  handler_name (contract_handler h)
  {
    return h == nullptr   ? "<null>"    :
           h == &first    ? "first"     :
           h == &second   ? "second"    : "<unknown>";
  }

  int
  print_handlers ()
  {
    cout << handler_name (set_contract_handler (&first))   << endl;
    cout << handler_name (set_contract_handler (&second))  << endl;
    cout << handler_name (set_contract_handler (nullptr))  << endl;
    cout << handler_name (set_contract_handler (nullptr))  << endl;

    return 0;
  }

  // Report a violation using the handler installed by the caller.
  //
  void
  report_violation ()
  {
    try
    {
      contract_fail (contract_kind::assertion,
                     "thread.cxx",
                     1,
                     "thread_func",
                     "handled");
    }
    catch (const violation& e)
    {
      print (e.v, true);
    }
  }

  int
  thread_handler ()
  {
    set_contract_handler (&throwing_handler);

    // Note that threads are not available in every configuration (for
    // example, GCC configured with the win32 threads model). In this
    // case perform the check in the current thread, which still
    // exercises the handler, only not its visibility from another
    // thread.
    //
#ifdef __cpp_lib_jthread
    {
      jthread t (&report_violation); // Joins in the destructor.
    }
#else
    report_violation ();
#endif

    assert (handler_calls == 1);

    return 0;
  }

  // fail <kind> <file> <line> <function> <expr> [<message>]
  //
  int
  fail (int argc, const char* argv[])
  {
    assert (argc == 5 || argc == 6);

    set_contract_handler (&throwing_handler);

    try
    {
      // Note: does not return normally.
      //
      contract_fail (kind (argv[0]),
                     text (argv[1]),
                     number (argv[2]),
                     text (argv[3]),
                     text (argv[4]),
                     argc == 6 ? text (argv[5]) : nullptr);
    }
    catch (const violation& e)
    {
      assert (handler_calls == 1);
      print (e.v, true);
    }

    return 0;
  }

  // report [-r] <kind> <file> <line> <function> <expr> [<message>]
  //
  int
  report (int argc, const char* argv[])
  {
    bool r (argc != 0 && strcmp (argv[0], "-r") == 0);

    if (r)
    {
      --argc;
      ++argv;
    }

    assert (argc == 5 || argc == 6);

    trap_abort ();

    if (r)
      set_contract_handler (&returning_handler);

    // This reports the violation to stderr and terminates the process,
    // which the SIGABRT handler installed above turns into exiting with
    // the abort_status code.
    //
    contract_fail (kind (argv[0]),
                   text (argv[1]),
                   number (argv[2]),
                   text (argv[3]),
                   text (argv[4]),
                   argc == 6 ? text (argv[5]) : nullptr);
  }

  // check <level> <check> (true|false)
  //
  int
  check (int argc, const char* argv[])
  {
    assert (argc == 3);

    const test::level* l (find_level (argv[0]));

    if (l == nullptr)
    {
      cerr << "error: unknown checking level '" << argv[0] << "'" << endl;
      return 1;
    }

    const test::check* c (find_check (*l, argv[1]));

    if (c == nullptr)
    {
      cerr << "error: unknown check '" << argv[1] << "'" << endl;
      return 1;
    }

    bool ok (boolean (argv[2]));

    // When contract checking is off some of the checks describe the
    // expression to the optimizer instead of testing it, which makes
    // failing them undefined behavior.
    //
    if (l->value == LIBIW4X_CONTRACT_OFF &&
        c->assume                           &&
        (!ok || c->always_fails))
    {
      cerr << "error: check '" << c->name << "' cannot fail at the '"
           << l->name << "' level" << endl;
      return 1;
    }

    set_contract_handler (&throwing_handler);

    unsigned line (0);

    try
    {
      c->function (ok, line);
    }
    catch (const violation& e)
    {
      // The violation must be reported at the location of the check.
      //
      assert (e.v.file != nullptr &&
              strcmp (base_name (e.v.file), base_name (l->file)) == 0);
      assert (e.v.line == line);

      // A check must not be reported more than once.
      //
      assert (handler_calls == 1);

      print (e.v, false);
      return 0;
    }
    catch (const test::unwind&)
    {
      cout << "unwound" << endl;
    }

    assert (handler_calls == 0);

    return 0;
  }

  int
  usage (const char* n)
  {
    cerr << "usage: " << n << " (kinds|levels|handler|thread|fail|report|"
         << "check) [<arg>...]" << endl;

    return 1;
  }
}

int
main (int argc, const char* argv[])
{
  assert (argc > 0);

  cout.exceptions (ios::failbit | ios::badbit);
  cerr.exceptions (ios::failbit | ios::badbit);

  if (argc < 2)
    return usage (argv[0]);

  string c (argv[1]);

  // The command arguments.
  //
  int          n (argc - 2);
  const char** a (argv + 2);

  if (c == "kinds")
  {
    assert (n == 0);
    return print_kinds ();
  }
  else if (c == "levels")
  {
    assert (n == 0);
    return print_levels ();
  }
  else if (c == "handler")
  {
    assert (n == 0);
    return print_handlers ();
  }
  else if (c == "thread")
  {
    assert (n == 0);
    return thread_handler ();
  }

  else if (c == "fail")   return fail (n, a);
  else if (c == "report") return report (n, a);
  else if (c == "check")  return check (n, a);

  return usage (argv[0]);
}
