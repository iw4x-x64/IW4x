#pragma once

// C Standard Library headers
//
extern "C"
{
  #include <io.h>
}

// Language support library
//
#include <initializer_list>

// Concepts library
//
#include <concepts>

// Diagnostics library
//
#include <cassert>
#include <exception>
#include <stdexcept>

// Memory management library
//
#include <memory>

// General utilities library
//
#include <functional>
#include <variant>

// Containers library
//
#include <array>
#include <vector>
#include <queue>

// Strings library
//
#include <string>
#include <string_view>

// Time library
//
#include <chrono>

// Input/output library
//
#include <iostream>

// Concurrency support library
//
#include <atomic>
#include <future>
#include <mutex>
#include <thread>

namespace iw4x
{
  // Language support
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
  using std::runtime_error;
  using std::terminate;

  // Memory management
  //
  using std::unique_ptr;
  using std::shared_ptr;
  using std::make_unique;

  // General utilities
  //
  using std::move_only_function;
  using std::variant;

  // Containers
  //
  using std::array;
  using std::vector;
  using std::queue;

  // Strings
  //
  using std::string;
  using std::string_view;

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
  using std::ios;
  using std::ostringstream;
  using std::size_t;

  // Concurrency support
  //
  using std::atomic;
  using std::future;
  using std::future_status;
  using std::jthread;
  using std::launch;
  using std::lock_guard;
  using std::memory_order_acquire;
  using std::memory_order_relaxed;
  using std::memory_order_release;
  using std::mutex;
  using std::thread;
  using std::scoped_lock;
}
