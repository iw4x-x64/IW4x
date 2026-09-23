#include <libiw4x/xcurl/registry.hxx>

#include <libiw4x/logger.hxx>

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

      for (uint32_t i (0); i != max_transfers; ++i)
      {
        if (p.transfer_used[i])
          continue;

        transfer& t (p.transfers[i]);

        if (!t.open ())
          return nullptr;

        p.transfer_used[i] = true;
        return &t;
      }

      warn ("no room for another transfer, {} in use", max_transfers);
      return nullptr;
    }

    void
    close_transfer (transfer& t) noexcept
    {
      pool&      p (handles ());
      scope_lock l (p.mutex_);

      t.disown ();

      for (uint32_t i (0); i != max_transfers; ++i)
      {
        if (&p.transfers[i] == &t)
        {
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

      if (t < p.transfers || t >= p.transfers + max_transfers)
        return nullptr;

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

      for (uint32_t i (0); i != max_multis; ++i)
      {
        if (p.multi_used[i])
          continue;

        multi& m (p.multis[i]);

        if (!m.open ())
          return nullptr;

        p.multi_used[i] = true;
        return &m;
      }

      warn ("no room for another multi handle, {} in use", max_multis);
      return nullptr;
    }

    void
    close_multi (multi& m) noexcept
    {
      pool&      p (handles ());
      scope_lock l (p.mutex_);

      m.disown ();

      for (uint32_t i (0); i != max_multis; ++i)
      {
        if (&p.multis[i] == &m)
        {
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

      if (m < p.multis || m >= p.multis + max_multis)
        return nullptr;

      return m->ours () ? m : nullptr;
    }
  }
}
