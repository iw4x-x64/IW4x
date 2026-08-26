#include <cstring>

#include <libiw4x/xcurl/xcurl.hxx>

#undef NDEBUG
#include <cassert>

using namespace iw4x;
using namespace iw4x::xcurl;

int
main ()
{
  assert (xcurl_global_init (CURL_GLOBAL_ALL) == CURLE_OK);

  {
    CURL* a (xcurl_easy_init ());
    CURL* b (xcurl_easy_init ());

    assert (a != nullptr && b != nullptr && a != b);

    xcurl_easy_cleanup (a);
    xcurl_easy_cleanup (b);
  }

  {
    CURL* h (xcurl_easy_init ());

    assert (xcurl_easy_setopt (h, CURLOPT_URL, "https://example.com/") ==
            CURLE_OK);
    assert (xcurl_easy_setopt (h, CURLOPT_TIMEOUT_MS, 5000L) == CURLE_OK);
    assert (xcurl_easy_setopt (h, CURLOPT_CUSTOMREQUEST, "PATCH") ==
            CURLE_OK);
    assert (xcurl_easy_setopt (h, CURLOPT_WRITEFUNCTION, nullptr) ==
            CURLE_OK);
    assert (xcurl_easy_setopt (h, CURLOPT_INFILESIZE_LARGE,
                               static_cast<curl_off_t> (7)) == CURLE_OK);

    long code (1);
    assert (xcurl_easy_getinfo (h, CURLINFO_RESPONSE_CODE, &code) ==
            CURLE_OK);
    assert (code == 0);

    char* url (nullptr);
    assert (xcurl_easy_getinfo (h, CURLINFO_EFFECTIVE_URL, &url) ==
            CURLE_OK);

    xcurl_easy_cleanup (h);
  }

  {
    int  nothing (0);
    long code (0);

    assert (xcurl_easy_setopt (nullptr, CURLOPT_URL, "x") ==
            CURLE_BAD_FUNCTION_ARGUMENT);
    assert (xcurl_easy_setopt (&nothing, CURLOPT_URL, "x") ==
            CURLE_BAD_FUNCTION_ARGUMENT);

    assert (xcurl_easy_getinfo (nullptr, CURLINFO_RESPONSE_CODE, &code) ==
            CURLE_BAD_FUNCTION_ARGUMENT);
    assert (xcurl_easy_getinfo (&nothing, CURLINFO_RESPONSE_CODE, &code) ==
            CURLE_BAD_FUNCTION_ARGUMENT);

    xcurl_easy_cleanup (nullptr);
    xcurl_easy_cleanup (&nothing);
  }

  {
    assert (std::strcmp (xcurl_easy_strerror (CURLE_OK), "No error") == 0);
    assert (xcurl_easy_strerror (CURLE_BAD_FUNCTION_ARGUMENT) != nullptr);
  }

  {
    CURL* h (xcurl_easy_init ());

    char* e (xcurl_easy_escape (h, "a b&c", 0));

    assert (e != nullptr);
    assert (std::strcmp (e, "a%20b%26c") == 0);

    int   n (0);
    char* u (xcurl_easy_unescape (h, e, 0, &n));

    assert (u != nullptr);
    assert (std::strcmp (u, "a b&c") == 0);
    assert (n == 5);

    xcurl_free (e);
    xcurl_free (u);

    xcurl_easy_cleanup (h);
  }

  {
    curl_slist* l (nullptr);

    l = xcurl_slist_append (l, "A: 1");
    l = xcurl_slist_append (l, "B: 2");

    assert (l != nullptr);
    assert (std::strcmp (l->data, "A: 1") == 0);
    assert (l->next != nullptr);
    assert (std::strcmp (l->next->data, "B: 2") == 0);
    assert (l->next->next == nullptr);

    xcurl_slist_free_all (l);
  }

  {
    CURLM* m (xcurl_multi_init ());

    assert (m != nullptr);

    CURL* h (xcurl_easy_init ());

    assert (xcurl_easy_setopt (h, CURLOPT_URL, "https://example.com/") ==
            CURLE_OK);

    assert (xcurl_multi_add_handle (m, h) == CURLM_OK);
    assert (xcurl_multi_remove_handle (m, h) == CURLM_OK);

    int running (-1);
    assert (xcurl_multi_perform (m, &running) == CURLM_OK);
    assert (running == 0);

    int left (-1);
    assert (xcurl_multi_info_read (m, &left) == nullptr);
    assert (left == 0);

    int ready (-1);
    assert (xcurl_multi_poll (m, nullptr, 0, 0, &ready) == CURLM_OK);

    assert (xcurl_multi_cleanup (m) == CURLM_OK);

    xcurl_easy_cleanup (h);
  }

  {
    int nothing (0);
    int n (0);

    assert (xcurl_multi_add_handle (nullptr, nullptr) ==
            CURLM_BAD_EASY_HANDLE);
    assert (xcurl_multi_remove_handle (&nothing, &nothing) ==
            CURLM_BAD_EASY_HANDLE);
    assert (xcurl_multi_perform (nullptr, &n) == CURLM_BAD_HANDLE);
    assert (xcurl_multi_poll (nullptr, nullptr, 0, 0, &n) ==
            CURLM_BAD_HANDLE);
    assert (xcurl_multi_info_read (nullptr, &n) == nullptr);
    assert (xcurl_multi_cleanup (nullptr) == CURLM_BAD_HANDLE);
  }

  {
    CURL* h (xcurl_easy_init ());

    assert (h != nullptr);

    xcurl_easy_cleanup (h);

    CURL* again (xcurl_easy_init ());

    assert (again == h);

    xcurl_easy_cleanup (again);
  }

  xcurl_global_cleanup ();
}
