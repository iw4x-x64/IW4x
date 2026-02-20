#pragma once

// C Standard Library headers
//
extern "C"
{
  #include <io.h>
}

// Language support library
//
#include <cstdint>
#include <initializer_list>

// Concepts library
//
#include <concepts>

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
#include <functional>
#include <utility>

// Containers library
//
#include <array>
#include <unordered_map>
#include <vector>

// Strings library
//
#include <cstring>
#include <string>
#include <string_view>

// Time library
//
#include <chrono>

// Input/output library
//
#include <cstdio>
#include <iostream>
#include <sstream>

// Concurrency support library
//
#include <atomic>
#include <future>
#include <mutex>
#include <thread>

namespace iw4x
{
  // Language support library
  //
  using std::initializer_list;

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

  // Concepts
  //
  using std::convertible_to;

  // Diagnostics
  //
  using std::system_category;
  using std::invalid_argument;
  using std::logic_error;
  using std::out_of_range;
  using std::runtime_error;
  using std::system_error;

  // Memory management
  //
  using std::make_unique;
  using std::unique_ptr;
  using std::shared_ptr;

  // General utilities
  //
  using std::forward;
  using std::in_range;
  using std::function;
  using std::move_only_function;

  // Containers
  //
  using std::array;
  using std::unordered_map;
  using std::vector;

  // Time
  //
  using namespace std::chrono_literals;
  using std::chrono::seconds;

  // Strings library
  //
  using std::string;
  using std::string_view;

  // Input/output
  //
  using std::cerr;
  using std::cout;
  using std::dec;
  using std::endl;
  using std::hex;
  using std::ios;
  using std::size_t;
  using std::ostringstream;

  // Concurrency support
  //
  using std::call_once;
  using std::jthread;
  using std::launch;
  using std::thread;
  using std::atomic;
  using std::future_status;
  using std::future;
  using std::once_flag;
  using std::memory_order_acquire;
  using std::memory_order_release;
  using std::memory_order_relaxed;
}
