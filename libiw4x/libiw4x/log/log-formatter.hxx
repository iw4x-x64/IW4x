#include <quill/DeferredFormatCodec.h>
#include <quill/bundled/fmt/base.h>

#include <libiw4x/import.hxx>

template <>
struct fmtquill::formatter<iw4x::XAssetType>
{
  constexpr auto
  parse (format_parse_context& ctx)
  {
    return ctx.begin ();
  }

  auto
  format (const iw4x::XAssetType& t, format_context& ctx) const
  {
    return format_to (ctx.out (), "{}", static_cast<int> (t));
  }
};
