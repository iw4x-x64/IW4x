#include <libiw4x/xcurl/multi.hxx>

#include <span>
#include <algorithm>

#include <libiw4x/logger.hxx>
#include <libiw4x/contract.hxx>

#include <libiw4x/xcurl/registry.hxx>

using namespace std;

namespace iw4x
{
  namespace xcurl
  {
    using namespace gdk;

    bool multi::
    open () noexcept
    {
      close ();

      handle_ = curl_multi_init ();

      if (handle_ == nullptr)
        return false;

      signature_ = multi_signature;
      return true;
    }

    void multi::
    close () noexcept
    {
      transfer* ls[capacity];
      uint32_t n (0);

      {
        scope_lock l (mutex_);

        n = locals_;

        copy_n (local_, n, ls);

        locals_ = 0;
        head_ = 0;
        count_ = 0;

        answered_.store (0, memory_order_release);
      }

      for (transfer* t: span (ls, n))
        curl_multi_remove_handle (handle_, t->easy ());

      if (handle_ != nullptr)
      {
        curl_multi_cleanup (handle_);
        handle_ = nullptr;
      }
    }

    CURLMcode multi::
    add (transfer& t) noexcept
    {
      LIBIW4X_PRE (handle_ != nullptr);
      LIBIW4X_PRE (t.ours ());

      if (!t.begin ())
        return CURLM_INTERNAL_ERROR;

      if (!t.local ())
        return curl_multi_add_handle (handle_, t.easy ());

      scope_lock l (mutex_);

      if (locals_ == capacity)
      {
        warn ("no room for another answered transfer, {} in flight",
              capacity);

        return CURLM_OUT_OF_MEMORY;
      }

      local_[locals_++] = &t;

      answered_.store (locals_, memory_order_release);

      return CURLM_OK;
    }

    CURLMcode multi::
    remove (transfer& t) noexcept
    {
      LIBIW4X_PRE (handle_ != nullptr);
      LIBIW4X_PRE (t.ours ());

      if (!t.local ())
        return curl_multi_remove_handle (handle_, t.easy ());

      scope_lock l (mutex_);

      transfer** e (local_ + locals_);
      transfer** i (find (local_, e, &t));

      if (i != e)
      {
        *i = local_[--locals_];

        answered_.store (locals_, memory_order_release);
      }

      return CURLM_OK;
    }

    bool multi::
    queue (transfer& t) noexcept
    {
      if (count_ == capacity)
        return false;

      CURLMsg& m (messages_[(head_ + count_) % capacity]);

      m.msg = CURLMSG_DONE;
      m.easy_handle = static_cast<CURL*> (&t);
      m.data.result = t.outcome ();

      ++count_;
      return true;
    }

    int multi::
    answered () noexcept
    {
      transfer* ls[capacity];
      uint32_t  k (0);

      {
        scope_lock l (mutex_);

        k = locals_;

        copy_n (local_, k, ls);
      }

      for (transfer* t: span (ls, k))
        t->deliver ();

      scope_lock l (mutex_);

      for (uint32_t i (0); i != locals_;)
      {
        transfer& t (*local_[i]);

        if (!t.delivered ())
        {
          ++i;
          continue;
        }

        if (!queue (t))
        {
          warn ("no room for another completion, {} outstanding", count_);
          ++i;
          continue;
        }

        local_[i] = local_[--locals_];
      }

      answered_.store (locals_, memory_order_release);

      return static_cast<int> (locals_);
    }

    CURLMcode multi::
    perform (int* running) noexcept
    {
      LIBIW4X_PRE (handle_ != nullptr);

      int n (0);

      CURLMcode r (curl_multi_perform (handle_, &n));

      if (answered_.load (memory_order_acquire) != 0)
        n += answered ();

      if (running != nullptr)
        *running = n;

      return r;
    }

    CURLMsg* multi::
    read (int* left) noexcept
    {
      LIBIW4X_PRE (handle_ != nullptr);

      scope_lock l (mutex_);

      if (count_ != 0)
      {
        current_ = messages_[head_];

        head_ = (head_ + 1) % capacity;
        --count_;

        if (left != nullptr)
          *left = static_cast<int> (count_);

        return &current_;
      }

      int n (0);

      CURLMsg* m (curl_multi_info_read (handle_, &n));

      if (m == nullptr)
      {
        if (left != nullptr)
          *left = 0;

        return nullptr;
      }

      current_ = *m;

      if (transfer* t = transfer_of (m->easy_handle))
        current_.easy_handle = static_cast<CURL*> (t);

      if (left != nullptr)
        *left = n;

      return &current_;
    }

    CURLMcode multi::
    poll (curl_waitfd* extra,
          unsigned int count,
          int timeout,
          int* ready) noexcept
    {
      LIBIW4X_PRE (handle_ != nullptr);

      if (answered_.load (memory_order_acquire) != 0)
        timeout = 0;

      return curl_multi_poll (handle_, extra, count, timeout, ready);
    }
  }
}
