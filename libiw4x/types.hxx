#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace iw4x
{
  using std::size_t;
  using std::nullptr_t;

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

  using std::ostringstream;
  using std::string;

  using std::array;
  using std::span;
  using std::unordered_map;
  using std::vector;

  using std::function;

  using std::unique_ptr;

  using std::mutex;
  using std::once_flag;
  using std::scoped_lock;

  using std::system_error;
  using std::runtime_error;
  using std::invalid_argument;
}
