#pragma once

// C Standard Library headers
//
extern "C"
{
  #include <io.h>
}

// Diagnostics library
//
#include <cassert>
#include <stdexcept>
#include <system_error>

// Memory management library
//
#include <memory>

// General utilities library
//
#include <utility>

// Strings library
//
#include <cstring>

// Input/output library
//
#include <iostream>

// Concurrency support library
//
#include <mutex>
#include <thread>

namespace iw4x
{
  // Diagnostics
  //
  using std::invalid_argument;
  using std::out_of_range;
  using std::runtime_error;
  using std::system_error;

  // Memory management
  //
  using std::make_unique;

  // General utilities
  //
  using std::forward;
  using std::in_range;

  // Input/output
  //
  using std::cerr;
  using std::cout;
  using std::dec;
  using std::endl;
  using std::hex;

  // Concurrency support
  //
  using std::call_once;
  using std::thread;
}
