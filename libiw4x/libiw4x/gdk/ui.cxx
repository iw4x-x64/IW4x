#include <libiw4x/gdk/ui.hxx>

#include <libiw4x/logger.hxx>

#include <libiw4x/gdk/path.hxx>
#include <libiw4x/gdk/sync.hxx>
#include <libiw4x/gdk/error.hxx>
#include <libiw4x/gdk/argument.hxx>

using namespace std;

namespace iw4x
{
  namespace gdk
  {
    namespace
    {
      inline constexpr size_t prompt_capacity (256);

      struct picker
      {
        mutex         mutex_;
        player_picker choose = nullptr;
      };

      picker&
      installed () noexcept
      {
        static picker p;
        return p;
      }

      const operation_id pick_id {"XGameUiShowPlayerPickerAsync"};

      class pick_operation: public operation
      {
      public:
        pick_operation (chars prompt,
                        const uint64_t* candidates,
                        size_t count,
                        size_t maximum)
            : prompt_ ("{}", prompt), count_ (count), maximum_ (maximum)
        {
          if (count_ == 0)
            return;

          if (!candidates_.resize (count_ * sizeof (uint64_t)))
            raise (E_OUTOFMEMORY, "no room for {} candidates", count_);

          __builtin_memcpy (candidates_.data (),
                            candidates,
                            candidates_.size ());
        }

        size_t
        work () override
        {
          player_picker choose (nullptr);

          {
            picker&    p (installed ());
            scope_lock l (p.mutex_);

            choose = p.choose;
          }

          if (choose == nullptr)
          {
            l1 ("no player picker is installed, so nobody was chosen");
            return 0;
          }

          if (maximum_ != 0 &&
              !chosen_.resize (maximum_ * sizeof (uint64_t)))
            raise (E_OUTOFMEMORY, "no room for {} choices", maximum_);

          chosen_count_ = choose (
            prompt_,
            reinterpret_cast<const uint64_t*> (candidates_.data ()),
            count_,
            reinterpret_cast<uint64_t*> (chosen_.data ()),
            maximum_);

          if (chosen_count_ > maximum_)
            chosen_count_ = maximum_;

          info ("the player chose {} of {}", chosen_count_, count_);

          return chosen_count_ * sizeof (uint64_t);
        }

        void
        result (size_t size, void* buffer) override
        {
          size_t n (chosen_count_ * sizeof (uint64_t));

          if (size < n)
            raise (insufficient_buffer,
                   "a {} byte buffer holds none of a {} byte selection",
                   size,
                   n);

          if (n != 0)
            __builtin_memcpy (&answer (static_cast<uint64_t*> (buffer)),
                              chosen_.data (),
                              n);
        }

        size_t
        count () const noexcept
        {
          return chosen_count_;
        }

      private:
        text<prompt_capacity> prompt_;
        blob                  candidates_;
        size_t                count_;
        size_t                maximum_;
        blob                  chosen_;
        size_t                chosen_count_ = 0;
      };
    }

    void
    pick_players_with (player_picker p) noexcept
    {
      picker&    i (installed ());
      scope_lock l (i.mutex_);

      i.choose = p;
    }

    HRESULT WINAPI xgame_ui::
    show_player_picker_async (void*,
                              async_block* b,
                              void* user,
                              const char* prompt,
                              uint32_t count,
                              const uint64_t* xuids,
                              uint32_t,
                              const void*,
                              uint32_t,
                              uint32_t maximum) noexcept
    {
      return guard (pick_id.name, [&] () -> HRESULT
      {
        chars p (prompt != nullptr ? prompt : "");

        l1 ("XGameUiShowPlayerPickerAsync for user {}: \"{}\", {} to choose "
            "from, at most {}",
            user,
            p.data (),
            count,
            maximum);

        if (count > max_candidates)
          raise_invalid ("{} candidates is more than a picker takes", count);

        if (xuids == nullptr)
          count = 0;

        begin (b,
               pick_id,
               make<pick_operation> (p, xuids, count, maximum));

        return S_OK;
      });
    }

    HRESULT WINAPI xgame_ui::
    show_player_picker_result (void*,
                               async_block* b,
                               uint32_t maximum,
                               uint64_t* out,
                               uint32_t* count) noexcept
    {
      return guard ("XGameUiShowPlayerPickerResult", [&] () -> HRESULT
      {
        auto* o (
          static_cast<pick_operation*> (pending_operation (b, pick_id)));

        if (o == nullptr)
          raise_invalid ("not a player picker operation of ours");

        size_t n (o->count ());

        if (n > maximum)
          n = maximum;

        HRESULT hr (result (b, pick_id, n * sizeof (uint64_t), out));

        if (SUCCEEDED (hr))
          answer (count) = static_cast<uint32_t> (n);

        return hr;
      });
    }
  }
}
