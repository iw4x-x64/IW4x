#pragma once

#include <cassert>
#include <functional>
#include <ios>
#include <iostream>
#include <memory>
#include <mutex>
#include <system_error>
#include <type_traits>

namespace iw4x
{
  using std::cerr;
  using std::cout;
  using std::endl;
  using std::ios;

  using std::forward;

  using std::make_unique;

  using std::call_once;

  using std::is_function_v;
  using std::remove_pointer_t;

  using std::system_category;
}
