#ifndef BEAKER_FRONTEND_THIRD_PARSER_HPP
#define BEAKER_FRONTEND_THIRD_PARSER_HPP

#include <beaker/frontend/parser.hpp>

namespace beaker
{
  /// Constructs a concrete syntax tree from a source file.
  struct Third_parser : Parser
  {
    using Parser::Parser;

    Syntax* parse_prefix_expression() override;
    
    Syntax* parse_parameter_group();
    Syntax* parse_parameter_list();
  };

} // namespace beaker

#endif
