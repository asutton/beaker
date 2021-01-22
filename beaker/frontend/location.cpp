#include <beaker/frontend/location.hpp>

#include <iostream>

namespace beaker
{
  std::ostream& operator<<(std::ostream& os, Source_location loc)
  {
    os << loc.line;
    if (loc.column != 0)
      os << ':' << loc.column;
    return os;
  }

} // namespace beaker
