#pragma once

#include <libiw4x/iw4x.hxx>
#include <libiw4x/export.hxx>

namespace iw4x
{
  // Low-level hook installation.
  //
  // We pass target by reference because the detour implementation often
  // overwrites it with the trampoline (pointer to the original function) so
  // we can call the original behavior later.
  //
  LIBIW4X_SYMEXPORT void
  detour (void*& t, void* s);

  // Type-erased wrapper.
  //
  // We really don't want to type reinterpret_cast<void*> (...) every single
  // time we hook a function. This template handles the ugly casting dance for
  // us.
  //
  inline void
  detour (auto& t, auto s)
  {
    void* pt (reinterpret_cast<void*> (t));
    void* ps (reinterpret_cast<void*> (s));

    detour (pt, ps);

    // Update the caller's pointer with the trampoline.
    //
    t = reinterpret_cast<decltype (t)> (pt);
  }
}
