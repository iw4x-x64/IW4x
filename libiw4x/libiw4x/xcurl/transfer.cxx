#include <libiw4x/xcurl/transfer.hxx>

#include <libiw4x/logger.hxx>

namespace iw4x
{
  namespace xcurl
  {
    bool transfer::
    open () noexcept
    {
      close ();

      easy_ = curl_easy_init ();

      if (easy_ == nullptr)
        return false;

      signature_ = transfer_signature;

      url_ = text<url_limit> ();
      method_ = text<method_limit> ("GET");

      write_ = nullptr;
      header_ = nullptr;
      write_data_ = nullptr;
      header_data_ = nullptr;

      local_ = false;
      delivered_ = false;
      outcome_ = CURLE_OK;

      answer_ = response ();

      return true;
    }

    void transfer::
    close () noexcept
    {
      if (easy_ != nullptr)
      {
        curl_easy_cleanup (easy_);
        easy_ = nullptr;
      }
    }

    CURLcode transfer::
    set (int o, long v) noexcept
    {
      switch (static_cast<option> (o))
      {
      case option::httpget:  method_ = text<method_limit> ("GET");  break;
      case option::post:     method_ = text<method_limit> ("POST"); break;
      case option::upload:   method_ = text<method_limit> ("PUT");  break;
      case option::nobody:   method_ = text<method_limit> ("HEAD"); break;
      default: break;
      }

      return curl_easy_setopt (easy_, static_cast<CURLoption> (o), v);
    }

    CURLcode transfer::
    set (int o, void* v) noexcept
    {
      switch (static_cast<option> (o))
      {
      case option::url:
        url_ = text<url_limit> ("{}",
                                static_cast<const char*> (v) != nullptr
                                  ? chars (static_cast<const char*> (v))
                                  : chars ());
        break;

      case option::customrequest:
        if (v != nullptr)
          method_ = text<method_limit> ("{}",
                                        chars (static_cast<const char*> (v)));
        break;

      case option::writedata:  write_data_ = v;  break;
      case option::headerdata: header_data_ = v; break;

      default: break;
      }

      return curl_easy_setopt (easy_, static_cast<CURLoption> (o), v);
    }

    CURLcode transfer::
    set (int o, generic_function v) noexcept
    {
      switch (static_cast<option> (o))
      {
      case option::writefunction:
        write_ = reinterpret_cast<curl_write_callback> (v);
        break;

      case option::headerfunction:
        header_ = reinterpret_cast<curl_write_callback> (v);
        break;

      default: break;
      }

      return curl_easy_setopt (easy_, static_cast<CURLoption> (o), v);
    }

    CURLcode transfer::
    get (int i, long& v) const noexcept
    {
      if (local_ && static_cast<easy_info> (i) == easy_info::response_code)
      {
        v = answer_.status;
        return CURLE_OK;
      }

      if (local_ && static_cast<easy_info> (i) == easy_info::os_errno)
      {
        v = 0;
        return CURLE_OK;
      }

      return curl_easy_getinfo (easy_, static_cast<CURLINFO> (i), &v);
    }

    bool transfer::
    begin () noexcept
    {
      local_ = false;
      delivered_ = false;
      outcome_ = CURLE_OK;

      if (answer (method_, url_, answer_))
      {
        l1 ("{} {} answered by this client", method_.c_str (),
            url_.c_str ());

        local_ = true;
        return true;
      }

      destination d;

      if (platform (url_, d))
      {
        text<url_limit> r (redirect (url_, d));

        l1 ("{} answered by the IW4x platform at {}",
            url_.c_str (),
            r.c_str ());

        if (curl_easy_setopt (easy_,
                              CURLOPT_URL,
                              r.c_str ()) != CURLE_OK)
          return false;

        url_ = r;
      }

      l2 ("{} {}", method_.c_str (), url_.c_str ());

      return true;
    }

    bool transfer::
    deliver () noexcept
    {
      if (delivered_)
        return true;

      delivered_ = true;

      auto line ([this] (const char* f, auto&&... a) noexcept
      {
        if (header_ == nullptr)
          return;

        text<body_limit> h (f, a...);

        header_ (const_cast<char*> (h.c_str ()), 1, h.size (), header_data_);
      });

      line ("HTTP/1.1 {} {}\r\n",
            answer_.status,
            answer_.status == 200 ? "OK" : "Error");

      line ("Content-Type: {}\r\n", answer_.type);
      line ("Content-Length: {}\r\n", answer_.body.size ());
      line ("\r\n");

      if (answer_.body.size () == 0)
        return true;

      if (write_ == nullptr)
      {
        warn ("{}: no write callback for {} bytes",
              url_.c_str (),
              answer_.body.size ());

        return true;
      }

      std::size_t n (write_ (const_cast<char*> (answer_.body.c_str ()),
                             1,
                             answer_.body.size (),
                             write_data_));

      if (n != answer_.body.size ())
        warn ("{}: the write callback took {} of {} bytes",
              url_.c_str (),
              n,
              answer_.body.size ());

      return true;
    }
  }
}
