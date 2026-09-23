#pragma once

#include <meta>
#include <cstddef>
#include <concepts>
#include <type_traits>

#include <curl/curl.h>

#include <libiw4x/export.hxx>

namespace iw4x
{
  namespace xcurl
  {
    enum class option: int
    {
      infilesize     = CURLOPT_INFILESIZE,
      verbose        = CURLOPT_VERBOSE,
      header         = CURLOPT_HEADER,
      nobody         = CURLOPT_NOBODY,
      upload         = CURLOPT_UPLOAD,
      post           = CURLOPT_POST,
      postfieldsize  = CURLOPT_POSTFIELDSIZE,
      httpget        = CURLOPT_HTTPGET,
      timeout_ms     = CURLOPT_TIMEOUT_MS,
      writedata      = CURLOPT_WRITEDATA,
      url            = CURLOPT_URL,
      readdata       = CURLOPT_READDATA,
      errorbuffer    = CURLOPT_ERRORBUFFER,
      httpheader     = CURLOPT_HTTPHEADER,
      headerdata     = CURLOPT_HEADERDATA,
      customrequest  = CURLOPT_CUSTOMREQUEST,
      writefunction  = CURLOPT_WRITEFUNCTION,
      readfunction   = CURLOPT_READFUNCTION,
      headerfunction = CURLOPT_HEADERFUNCTION,
      debugfunction  = CURLOPT_DEBUGFUNCTION
    };

    enum class easy_info: int
    {
      response_code = CURLINFO_RESPONSE_CODE,
      os_errno      = CURLINFO_OS_ERRNO
    };

    enum class argument: int
    {
      number   = CURLOPTTYPE_LONG / 10000,
      object   = CURLOPTTYPE_OBJECTPOINT / 10000,
      function = CURLOPTTYPE_FUNCTIONPOINT / 10000,
      offset   = CURLOPTTYPE_OFF_T / 10000,
      blob     = CURLOPTTYPE_BLOB / 10000
    };

    constexpr argument
    argument_of (int o) noexcept
    {
      return static_cast<argument> (o / 10000);
    }

    enum class result: int
    {
      number = CURLINFO_LONG / 0x100000,
      text   = CURLINFO_STRING / 0x100000,
      real   = CURLINFO_DOUBLE / 0x100000,
      list   = CURLINFO_SLIST / 0x100000,
      socket = CURLINFO_SOCKET / 0x100000,
      offset = CURLINFO_OFF_T / 0x100000
    };

    constexpr result
    result_of (int i) noexcept
    {
      return static_cast<result> (i / 0x100000);
    }

    using generic_function = void (*) ();

    template <typename E>
    concept recovered_enum = std::is_enum_v<E>;

    template <recovered_enum E>
    inline const char*
    name_of (E v) noexcept
    {
      template for (constexpr std::meta::info e:
                      std::define_static_array (
                        std::meta::enumerators_of (^^E)))
      {
        if (v == [: e :])
          return std::meta::identifier_of (e).data ();
      }

      return "<unrecovered>";
    }

    inline constexpr std::size_t url_limit (2048);
    inline constexpr std::size_t method_limit (16);

    inline constexpr std::size_t error_size (CURL_ERROR_SIZE);

    static_assert (error_size == 256, "recovered CURL_ERROR_SIZE");
    static_assert (sizeof (CURLMsg) == 24, "recovered CURLMsg size");
    static_assert (offsetof (CURLMsg, easy_handle) == 8,
                   "0x14036AB80 reads the easy handle at byte 8");
    static_assert (offsetof (CURLMsg, data) == 16,
                   "0x14036AB80 reads the result at byte 16");
  }
}

extern "C"
{
  LIBIW4X_SYMEXPORT CURLcode  xcurl_global_init (long);
  LIBIW4X_SYMEXPORT void      xcurl_global_cleanup ();

  LIBIW4X_SYMEXPORT CURL*     xcurl_easy_init ();
  LIBIW4X_SYMEXPORT CURLcode  xcurl_easy_setopt (CURL*, int, ...);
  LIBIW4X_SYMEXPORT CURLcode  xcurl_easy_getinfo (CURL*, int, ...);
  LIBIW4X_SYMEXPORT void      xcurl_easy_cleanup (CURL*);
  LIBIW4X_SYMEXPORT const char* xcurl_easy_strerror (CURLcode);
  LIBIW4X_SYMEXPORT char*     xcurl_easy_escape (CURL*, const char*, int);
  LIBIW4X_SYMEXPORT char*     xcurl_easy_unescape (CURL*,
                                                   const char*,
                                                   int,
                                                   int*);
  LIBIW4X_SYMEXPORT void      xcurl_free (void*);

  LIBIW4X_SYMEXPORT curl_slist* xcurl_slist_append (curl_slist*,
                                                    const char*);
  LIBIW4X_SYMEXPORT void      xcurl_slist_free_all (curl_slist*);

  LIBIW4X_SYMEXPORT CURLM*    xcurl_multi_init ();
  LIBIW4X_SYMEXPORT CURLMcode xcurl_multi_add_handle (CURLM*, CURL*);
  LIBIW4X_SYMEXPORT CURLMcode xcurl_multi_remove_handle (CURLM*, CURL*);
  LIBIW4X_SYMEXPORT CURLMcode xcurl_multi_perform (CURLM*, int*);
  LIBIW4X_SYMEXPORT CURLMcode xcurl_multi_poll (CURLM*,
                                                curl_waitfd*,
                                                unsigned int,
                                                int,
                                                int*);
  LIBIW4X_SYMEXPORT CURLMsg*  xcurl_multi_info_read (CURLM*, int*);
  LIBIW4X_SYMEXPORT CURLMcode xcurl_multi_cleanup (CURLM*);
}
