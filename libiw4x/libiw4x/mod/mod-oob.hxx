#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <libiw4x/export.hxx>
#include <libiw4x/import.hxx>

namespace iw4x
{
  namespace mod
  {
    using handler = std::function<void (const network_address&,
                                        const std::vector<std::string>&)>;

    class LIBIW4X_SYMEXPORT oob_module
    {
    public:
      oob_module ();

      bool
      dispatch (const network_address&, const message&);

      void
      register_handler (const std::string&, handler);

    private:
      std::unordered_map<std::string, handler> handlers_;
    };
  }
}
