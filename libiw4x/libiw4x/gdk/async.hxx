#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>

#include <libiw4x/gdk/types.hxx>
#include <libiw4x/gdk/vtable.hxx>
#include <libiw4x/gdk/task-queue.hxx>

namespace iw4x
{
  namespace gdk
  {
    struct async_block
    {
      task_queue* queue;
      void*       context;
      void      (*callback) (async_block*);
      void*       internal[4];
    };

    static_assert (sizeof (async_block) == 56, "recovered XAsyncBlock size");
    static_assert (offsetof (async_block, callback) == 16,
                   "recovered XAsyncBlock layout");

    struct operation_id
    {
      const char* name;

      const void*
      token () const noexcept
      {
        return this;
      }
    };

    class operation
    {
    public:
      virtual
      ~operation ();

      virtual std::size_t
      work () = 0;

      virtual void
      result (std::size_t size, void* buffer);
    };

    using operation_ptr = std::unique_ptr<operation>;

    class event
    {
    public:
      virtual
      ~event ();

      virtual void
      deliver () = 0;
    };

    using event_ptr = std::unique_ptr<event>;

    void
    begin (async_block*, const operation_id&, operation_ptr);

    HRESULT
    result (async_block*,
            const operation_id&,
            std::size_t size,
            void* buffer) noexcept;

    HRESULT
    result_size (async_block*, std::size_t*) noexcept;

    operation*
    pending_operation (async_block*, const operation_id&) noexcept;

    bool
    post (task_queue*, event_ptr) noexcept;

    struct xasync
    {
      static constexpr char label[] = "XAsync/XTaskQueue";

      static constexpr guid api
      {
        0x073B7DCB, 0x1FCF, 0x4030,
        {0x94, 0xBE, 0xE3, 0xC9, 0xEB, 0x62, 0x34, 0x28}
      };

      static constexpr guid id {api};

      static constexpr feature features[]
      {
        feature::async,
        feature::async_provider,
        feature::task_queue
      };

      [[= slot {0x18}]] static HRESULT WINAPI
      get_status (void*, async_block*, bool wait) noexcept;

      [[= slot {0x20}]] static HRESULT WINAPI
      get_result_size (void*, async_block*, std::size_t*) noexcept;

      [[= slot {0x28}]] static HRESULT WINAPI
      cancel (void*, async_block*) noexcept;

      [[= slot {0x38}]] static HRESULT WINAPI
      provider_begin (void*,
                      async_block*,
                      void* context,
                      const void* id,
                      const char* name,
                      HRESULT (*provider) (std::uint32_t,
                                           const void*)) noexcept;

      [[= slot {0x48}]] static HRESULT WINAPI
      schedule (void*, async_block*, std::uint32_t delay) noexcept;

      [[= slot {0x50}]] static HRESULT WINAPI
      complete (void*, async_block*, HRESULT, std::size_t) noexcept;

      [[= slot {0x58}]] static HRESULT WINAPI
      get_result (void*,
                  async_block*,
                  const void* id,
                  std::size_t size,
                  void* buffer,
                  std::size_t* used) noexcept;

      [[= slot {0xE8}]] static HRESULT WINAPI
      precall (void*) noexcept;

      [[= slot {0x60}]] static HRESULT WINAPI
      queue_create (void*,
                    dispatch_mode work,
                    dispatch_mode completion,
                    task_queue**) noexcept;

      [[= slot {0x68}]] static HRESULT WINAPI
      queue_create_composite (void*,
                              task_port* work,
                              task_port* completion,
                              task_queue**) noexcept;

      [[= slot {0x70}]] static HRESULT WINAPI
      queue_get_port (void*, task_queue*, port, task_port**) noexcept;

      [[= slot {0x78}]] static HRESULT WINAPI
      queue_duplicate_handle (void*, task_queue*, task_queue**) noexcept;

      [[= slot {0x80}]] static bool WINAPI
      queue_dispatch (void*, task_queue*, port, std::uint32_t) noexcept;

      [[= slot {0x88}]] static HRESULT WINAPI
      queue_close_handle (void*, task_queue*) noexcept;

      [[= slot {0x90}]] static HRESULT WINAPI
      queue_submit_callback (void*,
                             task_queue*,
                             port,
                             void* context,
                             void (*) (void*, bool)) noexcept;

      [[= slot {0x98}]] static HRESULT WINAPI
      queue_submit_delayed_callback (void*,
                                     task_queue*,
                                     port,
                                     std::uint32_t delay,
                                     void* context,
                                     void (*) (void*, bool)) noexcept;

      [[= slot {0xB0}]] static HRESULT WINAPI
      queue_terminate (void*,
                       task_queue*,
                       bool wait,
                       void* context,
                       void (*) (void*)) noexcept;

      [[= slot {0xC8}]] static bool WINAPI
      queue_get_current_process (void*, task_queue**) noexcept;

      [[= slot {0xD0}]] static HRESULT WINAPI
      queue_set_current_process (void*, task_queue*) noexcept;
    };
  }
}
