#include <cstdarg>

#include <libiw4x/logger.hxx>

#include <libiw4x/xcurl/xcurl.hxx>
#include <libiw4x/xcurl/registry.hxx>

namespace iw4x
{
  namespace xcurl
  {
    namespace
    {
      CURL*
      easy_of (CURL* h) noexcept
      {
        transfer* t (as_transfer (h));

        return t != nullptr ? t->easy () : nullptr;
      }
    }

    extern "C"
    {
      CURLcode
      xcurl_global_init (long flags)
      {
        CURLcode r (curl_global_init (flags));

        if (r != CURLE_OK)
          fail ("unable to initialize curl: {}", curl_easy_strerror (r));
        else
          info ("curl {} up, flags {:#x}",
                curl_version_info (CURLVERSION_NOW)->version,
                flags);

        return r;
      }

      void
      xcurl_global_cleanup ()
      {
        info ("curl down");

        curl_global_cleanup ();
      }

      CURL*
      xcurl_easy_init ()
      {
        return static_cast<CURL*> (open_transfer ());
      }

      CURLcode
      xcurl_easy_setopt (CURL* h, int o, ...)
      {
        transfer* t (as_transfer (h));

        if (t == nullptr)
          return CURLE_BAD_FUNCTION_ARGUMENT;

        va_list a;
        va_start (a, o);

        CURLcode r (CURLE_OK);

        switch (argument_of (o))
        {
        case argument::number:
          r = t->set (o, va_arg (a, long));
          break;

        case argument::object:
        case argument::blob:
          r = t->set (o, va_arg (a, void*));
          break;

        case argument::function:
          r = t->set (o, va_arg (a, generic_function));
          break;

        case argument::offset:
          r = curl_easy_setopt (t->easy (),
                                static_cast<CURLoption> (o),
                                va_arg (a, curl_off_t));
          break;

        default:
          r = CURLE_BAD_FUNCTION_ARGUMENT;
          break;
        }

        va_end (a);

        if (r != CURLE_OK)
          warn ("{} refused: {}",
                name_of (static_cast<option> (o)),
                curl_easy_strerror (r));

        return r;
      }

      CURLcode
      xcurl_easy_getinfo (CURL* h, int i, ...)
      {
        transfer* t (as_transfer (h));

        if (t == nullptr)
          return CURLE_BAD_FUNCTION_ARGUMENT;

        va_list a;
        va_start (a, i);

        CURLcode r (CURLE_OK);

        switch (result_of (i))
        {
        case result::number:
          r = t->get (i, *va_arg (a, long*));
          break;

        case result::text:
          r = curl_easy_getinfo (t->easy (),
                                 static_cast<CURLINFO> (i),
                                 va_arg (a, char**));
          break;

        case result::real:
          r = curl_easy_getinfo (t->easy (),
                                 static_cast<CURLINFO> (i),
                                 va_arg (a, double*));
          break;

        case result::list:
          r = curl_easy_getinfo (t->easy (),
                                 static_cast<CURLINFO> (i),
                                 va_arg (a, curl_slist**));
          break;

        case result::socket:
          r = curl_easy_getinfo (t->easy (),
                                 static_cast<CURLINFO> (i),
                                 va_arg (a, curl_socket_t*));
          break;

        case result::offset:
          r = curl_easy_getinfo (t->easy (),
                                 static_cast<CURLINFO> (i),
                                 va_arg (a, curl_off_t*));
          break;

        default:
          r = CURLE_BAD_FUNCTION_ARGUMENT;
          break;
        }

        va_end (a);

        if (r != CURLE_OK)
          warn ("{} refused: {}",
                name_of (static_cast<easy_info> (i)),
                curl_easy_strerror (r));

        return r;
      }

      void
      xcurl_easy_cleanup (CURL* h)
      {
        if (transfer* t = as_transfer (h))
          close_transfer (*t);
      }

      const char*
      xcurl_easy_strerror (CURLcode c)
      {
        return curl_easy_strerror (c);
      }

      char*
      xcurl_easy_escape (CURL* h, const char* s, int n)
      {
        return curl_easy_escape (easy_of (h), s, n);
      }

      char*
      xcurl_easy_unescape (CURL* h, const char* s, int n, int* out)
      {
        return curl_easy_unescape (easy_of (h), s, n, out);
      }

      void
      xcurl_free (void* p)
      {
        curl_free (p);
      }

      curl_slist*
      xcurl_slist_append (curl_slist* head, const char* s)
      {
        return curl_slist_append (head, s);
      }

      void
      xcurl_slist_free_all (curl_slist* head)
      {
        curl_slist_free_all (head);
      }

      CURLM*
      xcurl_multi_init ()
      {
        return static_cast<CURLM*> (open_multi ());
      }

      CURLMcode
      xcurl_multi_add_handle (CURLM* mh, CURL* eh)
      {
        multi*    m (as_multi (mh));
        transfer* t (as_transfer (eh));

        if (m == nullptr || t == nullptr)
          return CURLM_BAD_EASY_HANDLE;

        return m->add (*t);
      }

      CURLMcode
      xcurl_multi_remove_handle (CURLM* mh, CURL* eh)
      {
        multi*    m (as_multi (mh));
        transfer* t (as_transfer (eh));

        if (m == nullptr || t == nullptr)
          return CURLM_BAD_EASY_HANDLE;

        return m->remove (*t);
      }

      CURLMcode
      xcurl_multi_perform (CURLM* mh, int* running)
      {
        multi* m (as_multi (mh));

        if (m == nullptr)
          return CURLM_BAD_HANDLE;

        return m->perform (running);
      }

      CURLMcode
      xcurl_multi_poll (CURLM* mh,
                        curl_waitfd* extra,
                        unsigned int count,
                        int timeout,
                        int* ready)
      {
        multi* m (as_multi (mh));

        if (m == nullptr)
          return CURLM_BAD_HANDLE;

        return m->poll (extra, count, timeout, ready);
      }

      CURLMsg*
      xcurl_multi_info_read (CURLM* mh, int* left)
      {
        multi* m (as_multi (mh));

        if (m == nullptr)
          return nullptr;

        return m->read (left);
      }

      CURLMcode
      xcurl_multi_cleanup (CURLM* mh)
      {
        multi* m (as_multi (mh));

        if (m == nullptr)
          return CURLM_BAD_HANDLE;

        m->close ();

        close_multi (*m);

        return CURLM_OK;
      }
    }
  }
}
