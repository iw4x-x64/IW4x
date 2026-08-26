#include <cstring>

#include <libiw4x/xcurl/xcurl.hxx>

#undef NDEBUG
#include <cassert>

using namespace iw4x;
using namespace iw4x::xcurl;

int
main ()
{
  static_assert (static_cast<int> (option::url) == CURLOPT_URL);
  static_assert (static_cast<int> (option::timeout_ms) == CURLOPT_TIMEOUT_MS);
  static_assert (static_cast<int> (option::customrequest) ==
                 CURLOPT_CUSTOMREQUEST);
  static_assert (static_cast<int> (easy_info::response_code) ==
                 CURLINFO_RESPONSE_CODE);
  static_assert (static_cast<int> (easy_info::os_errno) == CURLINFO_OS_ERRNO);

  static_assert (error_size == CURL_ERROR_SIZE);
  static_assert (url_limit >= 2048);

  auto named = [] (auto v, const char* e)
  {
    return std::strcmp (name_of (v), e) == 0;
  };

  assert (named (option::url, "url"));
  assert (named (option::customrequest, "customrequest"));
  assert (named (option::writefunction, "writefunction"));
  assert (named (option::infilesize, "infilesize"));
  assert (named (option::debugfunction, "debugfunction"));
  assert (named (static_cast<option> (99999), "<unrecovered>"));

  assert (named (easy_info::response_code, "response_code"));
  assert (named (easy_info::os_errno, "os_errno"));
  assert (named (static_cast<easy_info> (0), "<unrecovered>"));

  assert (argument_of (CURLOPT_TIMEOUT_MS) == argument::number);
  assert (argument_of (CURLOPT_URL) == argument::object);
  assert (argument_of (CURLOPT_WRITEFUNCTION) == argument::function);
  assert (argument_of (CURLOPT_INFILESIZE_LARGE) == argument::offset);
  assert (argument_of (CURLOPT_SSLCERT_BLOB) == argument::blob);

  assert (result_of (CURLINFO_RESPONSE_CODE) == result::number);
  assert (result_of (CURLINFO_EFFECTIVE_URL) == result::text);
  assert (result_of (CURLINFO_TOTAL_TIME) == result::real);
  assert (result_of (CURLINFO_COOKIELIST) == result::list);
  assert (result_of (CURLINFO_ACTIVESOCKET) == result::socket);
  assert (result_of (CURLINFO_SIZE_UPLOAD_T) == result::offset);
}
