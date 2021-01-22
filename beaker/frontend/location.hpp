#ifndef BEAKER_FRONTEND_LOCATION_HPP
#define BEAKER_FRONTEND_LOCATION_HPP

#include <iosfwd>

namespace beaker
{
  /// Represents a location in a source file.
  ///
  /// FIXME: This needs to move to a higher-level library so that source
  /// locations can appear in ASTs. Also, we'll eventually need to bind
  /// locations to source files, possibly through modules. There are a bunch
  /// of techniques we can use to minimize the size of this object.
  struct Source_location
  {
    std::size_t line;
    std::size_t column;
  };

  std::ostream& operator<<(std::ostream& os, Source_location loc);

} // namespace beaker

#endif
