// file      : libbuild2/version.hxx.in -*- C++ -*-
// license   : MIT; see accompanying LICENSE file

#ifndef LIBBUILD2_VERSION // Note: using the version macro itself.

// The numeric version format is AAAAABBBBBCCCCCDDDE where:
//
// AAAAA - major version number
// BBBBB - minor version number
// CCCCC - bugfix version number
// DDD   - alpha / beta (DDD + 500) version number
// E     - final (0) / snapshot (1)
//
// When DDDE is not 0, 1 is subtracted from AAAAABBBBBCCCCC. For example:
//
// Version      AAAAABBBBBCCCCCDDDE
//
// 0.1.0        0000000001000000000
// 0.1.2        0000000001000020000
// 1.2.3        0000100002000030000
// 2.2.0-a.1    0000200001999990010
// 3.0.0-b.2    0000299999999995020
// 2.2.0-a.1.z  0000200001999990011

// NOTE: remember to also update "fake" bootstrap values in utility.hxx if
// changing anything here.

#define LIBBUILD2_VERSION       17999990001ULL
#define LIBBUILD2_VERSION_STR   "0.18.0-a.0.20260217121713.c80534ed43dc"
#define LIBBUILD2_VERSION_ID    "0.18.0-a.0.c80534ed43dc"
#define LIBBUILD2_VERSION_FULL  "0.18.0-a.0.20260217121713.c80534ed43dc"

#define LIBBUILD2_VERSION_MAJOR 0
#define LIBBUILD2_VERSION_MINOR 18
#define LIBBUILD2_VERSION_PATCH 0

#define LIBBUILD2_PRE_RELEASE   true

#define LIBBUILD2_SNAPSHOT      20260217121713ULL
#define LIBBUILD2_SNAPSHOT_ID   "c80534ed43dc"

#include <libbutl/version.hxx>

#ifdef LIBBUTL_VERSION
#  if !((LIBBUTL_VERSION > 17999990001ULL || (LIBBUTL_VERSION == 17999990001ULL && LIBBUTL_SNAPSHOT >= 1ULL)) && LIBBUTL_VERSION < 17999990010ULL)
#    error incompatible libbutl version, libbutl [0.18.0-a.0.1 0.18.0-a.1) is required
#  endif
#endif

#endif // LIBBUILD2_VERSION
