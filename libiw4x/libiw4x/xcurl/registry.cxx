#include <libiw4x/xcurl/registry.hxx>

#include <algorithm>
#include <functional>

#include <libiw4x/logger.hxx>
#include <libiw4x/contract.hxx>

using namespace std;

namespace iw4x
{
  namespace xcurl
  {
    using namespace gdk;

    namespace
    {
      struct pool
      {
        mutex mutex_;

        transfer transfers[max_transfers];
        bool     transfer_used[max_transfers] {};

        multi    multis[max_multis];
        bool     multi_used[max_multis] {};
      };

      pool&
      handles () noexcept
      {
        static pool p;
        return p;
      }
    }

    transfer*
    open_transfer () noexcept
    {
      pool&      p (handles ());
      scope_lock l (p.mutex_);

      bool* u (ranges::find (p.transfer_used, false));

      if (u == ranges::end (p.transfer_used))
      {
        warn ("no room for another transfer, {} in use", max_transfers);
        return nullptr;
      }

      transfer& t (p.transfers[u - p.transfer_used]);

      if (!t.open ())
        return nullptr;

      *u = true;
      return &t;
    }

    void
    close_transfer (transfer& t) noexcept
    {
      pool&      p (handles ());
      scope_lock l (p.mutex_);

      LIBIW4X_PRE (t.ours ());

      t.disown ();

      for (uint32_t i (0); i != max_transfers; ++i)
      {
        if (&p.transfers[i] == &t)
        {
          LIBIW4X_ASSERT (p.transfer_used[i]);

          p.transfer_used[i] = false;
          break;
        }
      }
    }

    transfer*
    as_transfer (CURL* h) noexcept
    {
      if (h == nullptr)
        return nullptr;

      pool& p (handles ());

      auto* t (static_cast<transfer*> (h));

      less<> before;

      if (before (t, p.transfers) || !before (t, p.transfers + max_transfers))
      {
        return nullptr;
      }

      return t->ours () ? t : nullptr;
    }

    transfer*
    transfer_of (CURL* easy) noexcept
    {
      if (easy == nullptr)
        return nullptr;

      pool&      p (handles ());
      scope_lock l (p.mutex_);

      for (uint32_t i (0); i != max_transfers; ++i)
      {
        if (p.transfer_used[i] && p.transfers[i].easy () == easy)
          return &p.transfers[i];
      }

      return nullptr;
    }

    multi*
    open_multi () noexcept
    {
      pool&      p (handles ());
      scope_lock l (p.mutex_);

      bool* u (ranges::find (p.multi_used, false));

      if (u == ranges::end (p.multi_used))
      {
        warn ("no room for another multi handle, {} in use", max_multis);
        return nullptr;
      }

      multi& m (p.multis[u - p.multi_used]);

      if (!m.open ())
        return nullptr;

      *u = true;
      return &m;
    }

    void
    close_multi (multi& m) noexcept
    {
      pool&      p (handles ());
      scope_lock l (p.mutex_);

      LIBIW4X_PRE (m.ours ());

      m.disown ();

      for (uint32_t i (0); i != max_multis; ++i)
      {
        if (&p.multis[i] == &m)
        {
          LIBIW4X_ASSERT (p.multi_used[i]);

          p.multi_used[i] = false;
          break;
        }
      }
    }

    multi*
    as_multi (CURLM* h) noexcept
    {
      if (h == nullptr)
        return nullptr;

      pool& p (handles ());

      auto* m (static_cast<multi*> (h));

      less<> before;

      if (before (m, p.multis) || !before (m, p.multis + max_multis))
      {
        return nullptr;
      }

      return m->ours () ? m : nullptr;
    }
  }
}
