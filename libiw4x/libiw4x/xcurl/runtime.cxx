// Copyright (c) the IW4x authors (see the AUTHORS file).
// SPDX-License-Identifier: GPL-3.0-only WITH AdditionRef-IW4x-Exception-1.1

#include <libiw4x/xcurl/runtime.hxx>

#include <libiw4x/gdk/user.hxx>

#include <libiw4x/xcurl/local.hxx>

using namespace std;

namespace iw4x
{
  namespace xcurl
  {
    using namespace gdk;

    void
    install () noexcept
    {
      serve (&served);
    }
  }
}
