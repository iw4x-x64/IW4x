#undef NDEBUG
#include <cassert>

#include <memory>
#include <cstring>

#include <libiw4x/gdk/async.hxx>
#include <libiw4x/gdk/types.hxx>
#include <libiw4x/gdk/error.hxx>

using namespace iw4x::gdk;

namespace
{
  const operation_id counted {"counted"};

  int destroyed (0);

  class counting: public operation
  {
  public:
    explicit
    counting (std::uint32_t v) noexcept: value_ (v) {}

    ~counting () override
    {
      ++destroyed;
    }

    std::size_t
    work () override
    {
      return sizeof (value_);
    }

    void
    result (std::size_t size, void* buffer) override
    {
      if (size < sizeof (value_))
        raise (insufficient_buffer, "no room");

      __builtin_memcpy (buffer, &value_, sizeof (value_));
    }

    std::uint32_t
    value () const noexcept
    {
      return value_;
    }

  private:
    std::uint32_t value_;
  };

  class failing: public operation
  {
  public:
    std::size_t
    work () override
    {
      raise (E_ACCESSDENIED, "no");
    }
  };

  int delivered (0);

  class note: public event
  {
  public:
    void
    deliver () override
    {
      ++delivered;
    }
  };

  int announced (0);

  void
  announce (async_block*) noexcept
  {
    ++announced;
  }

  task_queue*
  manual ()
  {
    task_queue* q (create_queue (dispatch_mode::manual,
                                 dispatch_mode::manual));

    assert (q != nullptr);
    return q;
  }
}

int
main ()
{
  task_queue* q (manual ());

  {
    async_block b {};
    b.queue = q;

    begin (&b, counted, std::make_unique<counting> (42u));

    assert (xasync::get_status (nullptr, &b, false) == pending);

    std::size_t n (0);
    assert (result_size (&b, &n) == pending);
    assert (xasync::get_result_size (nullptr, &b, &n) == pending);

    assert ((*q)[port::work].dispatch (0));

    assert (xasync::get_status (nullptr, &b, false) == S_OK);
    assert (result_size (&b, &n) == S_OK && n == sizeof (std::uint32_t));

    n = 0;
    assert (xasync::get_result_size (nullptr, &b, &n) == S_OK);
    assert (n == sizeof (std::uint32_t));

    assert (xasync::get_result_size (nullptr, &b, nullptr) == E_INVALIDARG);

    assert (xasync::get_status (nullptr, &b, true) == S_OK);

    auto* o (static_cast<counting*> (pending_operation (&b, counted)));
    assert (o != nullptr && o->value () == 42);

    std::uint32_t v (0);
    assert (result (&b, counted, sizeof (v), &v) == S_OK);
    assert (v == 42);

    assert (result (&b, counted, sizeof (v), &v) == E_INVALIDARG);
    assert (pending_operation (&b, counted) == nullptr);
  }

  {
    destroyed = 0;

    async_block b {};
    b.queue = q;

    begin (&b, counted, std::make_unique<counting> (7u));

    assert (xasync::cancel (nullptr, &b) == S_OK);

    assert ((*q)[port::work].dispatch (0));

    std::uint32_t v (0);
    assert (result (&b, counted, sizeof (v), &v) == E_INVALIDARG);

    assert (destroyed == 1);
  }

  {
    async_block b {};
    b.queue = q;

    begin (&b, counted, std::make_unique<failing> ());

    assert ((*q)[port::work].dispatch (0));

    assert (xasync::get_status (nullptr, &b, false) == E_ACCESSDENIED);
  }

  {
    async_block b {};
    b.queue = q;

    begin (&b, counted, std::make_unique<counting> (3u));

    assert (xasync::schedule (nullptr, &b, 0) == E_INVALIDARG);

    assert ((*q)[port::work].dispatch (0));
    assert (!(*q)[port::work].dispatch (0));

    std::uint32_t v (0);
    assert (result (&b, counted, sizeof (v), &v) == S_OK);
    assert (v == 3);
  }

  {
    announced = 0;

    async_block b {};
    b.queue = q;
    b.callback = &announce;

    begin (&b, counted, std::make_unique<counting> (1u));

    assert ((*q)[port::work].dispatch (0));
    assert (announced == 0);

    assert ((*q)[port::completion].dispatch (0));
    assert (announced == 1);

    std::uint32_t v (0);
    assert (result (&b, counted, sizeof (v), &v) == S_OK);
  }

  {
    async_block b {};
    b.queue = q;

    const void* id (&b);

    struct provider
    {
      static HRESULT
      run (std::uint32_t op, const void* d) noexcept
      {
        struct data
        {
          async_block* async;
          std::size_t  size;
          void*        buffer;
          void*        context;
        };

        const data& x (*static_cast<const data*> (d));

        switch (op)
        {
        case 0:
          return xasync::schedule (nullptr, x.async, 0);
        case 1:
          return xasync::complete (nullptr, x.async, S_OK, sizeof (int));
        case 2:
          *static_cast<int*> (x.buffer) = 99;
          return S_OK;
        default:
          return S_OK;
        }
      }
    };

    assert (xasync::provider_begin (nullptr,
                                    &b,
                                    nullptr,
                                    id,
                                    "provided",
                                    &provider::run) == S_OK);

    assert (xasync::get_status (nullptr, &b, false) == pending);

    assert ((*q)[port::work].dispatch (0));

    assert (xasync::get_status (nullptr, &b, false) == S_OK);

    int v (0);
    assert (xasync::get_result (nullptr, &b, id, sizeof (v), &v, 0) == S_OK);
    assert (v == 99);
  }

  {
    delivered = 0;

    assert (post (q, std::make_unique<note> ()));
    assert (delivered == 0);

    assert ((*q)[port::completion].dispatch (0));
    assert (delivered == 1);

    assert (!post (q, event_ptr ()));
  }

  {
    async_block b {};

    assert (xasync::get_status (nullptr, &b, false) == E_INVALIDARG);
    assert (xasync::cancel (nullptr, &b) == S_OK);

    std::size_t n (0);
    assert (result_size (&b, &n) == E_INVALIDARG);
  }

  {
    assert (xasync::precall (nullptr) == S_OK);
  }

  stop_queues ();
}
