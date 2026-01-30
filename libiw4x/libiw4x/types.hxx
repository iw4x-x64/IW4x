#pragma once

// Language support library
//
#include <cstdint>

// Diagnostics library
//
#include <stdexcept>
#include <system_error>

// Memory management library
//
#include <memory>

// General utilities library
//
#include <functional>

// Containers library
//
#include <array>
#include <unordered_map>
#include <vector>

// Strings library
//
#include <string>

// Input/output library
//
#include <cstdio>
#include <iostream>
#include <sstream>

// Concurrency support library
//
#include <mutex>
#include <future>

namespace iw4x
{
  // Language support
  //
  using std::int8_t;
  using std::int16_t;
  using std::int32_t;
  using std::int64_t;

  using std::intmax_t;
  using std::intptr_t;

  using std::uint8_t;
  using std::uint16_t;
  using std::uint32_t;
  using std::uint64_t;

  using std::uintmax_t;
  using std::uintptr_t;

  // Diagnostics
  //
  using std::invalid_argument;
  using std::out_of_range;
  using std::runtime_error;
  using std::system_error;

  // Memory management
  //
  using std::unique_ptr;

  // General utilities
  //
  using std::function;
  using std::forward;

  // Containers
  //
  using std::array;
  using std::unordered_map;
  using std::vector;

  // Strings
  //
  using std::string;

  // Input/output
  //
  using std::ios;
  using std::size_t;
  using std::ostringstream;

  // Concurrency support
  //
  using std::once_flag;
  using std::future;
  using std::future_status;
}
