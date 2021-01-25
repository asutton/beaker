#include <beaker/frontend/location.hpp>

#include <iostream>

namespace beaker
{
  std::ostream& operator<<(std::ostream& os, Source_location const& loc)
  {
    os << loc.line;
    if (loc.column != 0)
      os << ':' << loc.column;
    return os;
  }

  std::ostream& operator<<(std::ostream& os, Source_range const& range)
  {
    if (range.is_invalid())
      return os << "<invalid>";

    if (range.is_span()) {
      os << range.start.line << ':';
      if (range.is_location())
        os << range.start.column;
      else
        os << range.start.column << '-' << range.end.column;
    }
    else {
      os << range.start << ".." << range.end;
    }
    return os;
  }

} // namespace beaker
