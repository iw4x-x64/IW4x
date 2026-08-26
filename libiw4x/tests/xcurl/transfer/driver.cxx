#include <cstring>

#include <libiw4x/xcurl/xcurl.hxx>
#include <libiw4x/xcurl/local.hxx>

#undef NDEBUG
#include <cassert>

using namespace iw4x;
using namespace iw4x::xcurl;

namespace
{
  const char privacy_url[] =
    "https://privacy.xboxlive.com/users/xuid(9)/permission/validate"
    "?setting=Communications&target=xuid(7)";

  char        headers[1024];
  std::size_t headers_size (0);
  int         header_calls (0);

  char        body[1024];
  std::size_t body_size (0);

  void*       header_context (nullptr);
  void*       write_context (nullptr);

  std::size_t
  on_header (char* p, std::size_t s, std::size_t n, void* d) noexcept
  {
    header_context = d;
    ++header_calls;

    __builtin_memcpy (headers + headers_size, p, s * n);
    headers_size += s * n;

    return s * n;
  }

  std::size_t
  on_write (char* p, std::size_t s, std::size_t n, void* d) noexcept
  {
    write_context = d;

    __builtin_memcpy (body + body_size, p, s * n);
    body_size += s * n;

    return s * n;
  }

  int header_tag (0);
  int write_tag (0);
}

int
main ()
{
  assert (xcurl_global_init (CURL_GLOBAL_ALL) == CURLE_OK);

  CURLM* m (xcurl_multi_init ());
  assert (m != nullptr);

  {
    CURL* h (xcurl_easy_init ());

    assert (xcurl_easy_setopt (h, CURLOPT_URL, privacy_url) == CURLE_OK);
    assert (xcurl_easy_setopt (h, CURLOPT_HTTPGET, 1L) == CURLE_OK);

    assert (xcurl_easy_setopt (h, CURLOPT_HEADERFUNCTION, &on_header) ==
            CURLE_OK);
    assert (xcurl_easy_setopt (h, CURLOPT_HEADERDATA, &header_tag) ==
            CURLE_OK);
    assert (xcurl_easy_setopt (h, CURLOPT_WRITEFUNCTION, &on_write) ==
            CURLE_OK);
    assert (xcurl_easy_setopt (h, CURLOPT_WRITEDATA, &write_tag) ==
            CURLE_OK);

    assert (xcurl_multi_add_handle (m, h) == CURLM_OK);

    int running (-1);
    assert (xcurl_multi_perform (m, &running) == CURLM_OK);
    assert (running == 0);

    assert (header_calls == 4);
    assert (header_context == &header_tag);
    assert (write_context == &write_tag);

    headers[headers_size] = '\0';
    body[body_size] = '\0';

    assert (std::strcmp (headers,
                         "HTTP/1.1 200 OK\r\n"
                         "Content-Type: application/json\r\n"
                         "Content-Length: 18\r\n"
                         "\r\n") == 0);

    assert (std::strcmp (body, "{\"isAllowed\":true}") == 0);

    int      left (-1);
    CURLMsg* msg (xcurl_multi_info_read (m, &left));

    assert (msg != nullptr);
    assert (msg->msg == CURLMSG_DONE);
    assert (msg->easy_handle == h);
    assert (msg->data.result == CURLE_OK);
    assert (left == 0);

    assert (xcurl_multi_info_read (m, &left) == nullptr);

    long code (0);
    assert (xcurl_easy_getinfo (h, CURLINFO_RESPONSE_CODE, &code) ==
            CURLE_OK);
    assert (code == 200);

    long e (7);
    assert (xcurl_easy_getinfo (h, CURLINFO_OS_ERRNO, &e) == CURLE_OK);
    assert (e == 0);

    assert (xcurl_multi_remove_handle (m, h) == CURLM_OK);

    xcurl_easy_cleanup (h);
  }

  {
    headers_size = 0;
    body_size = 0;
    header_calls = 0;

    CURL* h (xcurl_easy_init ());

    assert (xcurl_easy_setopt (h, CURLOPT_URL, privacy_url) == CURLE_OK);
    assert (xcurl_easy_setopt (h, CURLOPT_HTTPGET, 1L) == CURLE_OK);

    assert (xcurl_multi_add_handle (m, h) == CURLM_OK);

    int running (-1);
    assert (xcurl_multi_perform (m, &running) == CURLM_OK);

    assert (header_calls == 0);
    assert (body_size == 0);

    int      left (-1);
    CURLMsg* msg (xcurl_multi_info_read (m, &left));

    assert (msg != nullptr && msg->easy_handle == h);

    assert (xcurl_multi_remove_handle (m, h) == CURLM_OK);

    xcurl_easy_cleanup (h);
  }

  {
    CURL* h (xcurl_easy_init ());

    assert (xcurl_easy_setopt (h, CURLOPT_URL,
                               "https://example.invalid/x") == CURLE_OK);

    assert (xcurl_multi_add_handle (m, h) == CURLM_OK);

    int running (-1);
    assert (xcurl_multi_perform (m, &running) == CURLM_OK);

    int      left (-1);
    CURLMsg* msg (xcurl_multi_info_read (m, &left));

    assert (msg == nullptr || msg->data.result != CURLE_OK);

    assert (xcurl_multi_remove_handle (m, h) == CURLM_OK);

    xcurl_easy_cleanup (h);
  }

  assert (xcurl_multi_cleanup (m) == CURLM_OK);

  xcurl_global_cleanup ();
}
