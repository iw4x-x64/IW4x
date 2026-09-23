#pragma once

namespace test
{
  // Contract checks performed by translation units that are built with
  // different LIBIW4X_CONTRACT values (see checks-impl.hxx for the
  // checks themselves).
  //
  // Each function performs exactly one contract check and stores the
  // source line of that check in the line argument, which allows the
  // driver to verify the location that is reported for a violation. The
  // ok argument specifies whether the checked expression holds.
  //
  using check_function = void (*) (bool ok, unsigned& line);

  // Thrown by the checks that leave the checked scope via an exception.
  //
  struct unwind {};

  struct check
  {
    const char*    name;
    check_function function;
    bool           assume;
    bool           always_fails;
  };

  struct level
  {
    const char*  name;   // Level name as spelled on the command line.
    unsigned     value;  // LIBIW4X_CONTRACT value.
    const char*  file;   // File the checks are defined in.
    const check* checks; // Array terminated by an entry with a NULL name.
  };

  extern const level level_off;
  extern const level level_default;
  extern const level level_audit;
}
