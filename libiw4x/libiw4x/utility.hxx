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

// Time library
//
#include <chrono>

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
  using std::system_category;

  // Memory management
  //
  using std::make_unique;

  // General utilities
  //
  using std::forward;
  using std::in_range;

  // Time
  //
  using namespace std::chrono_literals;
  using std::chrono::seconds;

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
  using std::launch;
}
