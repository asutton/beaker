#ifndef BEAKER_FRONTEND_SECOND_PARSER_HPP
#define BEAKER_FRONTEND_SECOND_PARSER_HPP

#include <beaker/frontend/parser.hpp>

namespace beaker
{
  /// Constructs a concrete syntax tree from a source file.
  struct Second_parser : Parser
  {
    using Parser::Parser;

    Syntax* parse_infix_expression() override;
    Syntax* parse_prefix_expression() override;
  };

} // namespace beaker

#endif
