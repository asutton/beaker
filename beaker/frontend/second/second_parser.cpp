#include <beaker/frontend/second/second_parser.hpp>

#include <beaker/frontend/syntax.hpp>

#include <iostream>
#include <sstream>

namespace beaker
{
  /// Parse an infix expression.
  ///
  ///   infix-expression:
  ///     logic-or-expression
  ///
  /// This language does not permit `->` as an infix operator because it
  /// is used as a suffix for function types in prefix-expressions.
  Syntax* Second_parser::parse_infix_expression()
  {
    return parse_logical_or_expression();
  }

  /// Find the matching offset of the current token.
  static std::size_t find_matching(Parser& p, Token::Kind k1, Token::Kind k2)
  {
    assert(p.lookahead() == k1);
    std::size_t braces = 0;
    std::size_t la = 0;
    while (Token::Kind k = p.lookahead(la)) {
      if (k == k1) {
        ++braces;
      }
      else if (k == k2) {
        --braces;
        if (braces == 0)
          break;
      }
      ++la;
    }
    return la;
  }

  // Returns true if the sequence of tokens would start a function type.
  // To determine this, starting at '(', we find the matching ')' and then
  // look for the trailing `->`.
  //
  // TODO: This would be cleaner if the enclosure stuff were in the parser
  // header.
  static bool starts_function_type(Parser& p)
  {
    std::size_t la = find_matching(p, Token::lparen_tok, Token::rparen_tok);
    return p.lookahead(la + 1) == Token::dash_greater_tok;
  }
  
  /// Parse a prefix expression.
  ///
  ///   prefix-expression:
  ///     postfix-expression
  ///     [ expression-list? ] prefix-expression
  ///     [ expression-list? ] => prefix-expression
  ///     ( expression-list? ) -> prefix-expression
  ///     const prefix-exprssion
  ///     ^ prefix-expression
  ///     - prefix-expression
  ///     + prefix-expression
  ///     not prefix-expression
  ///
  /// Note that the array notation is unambiguous only because there are no
  /// primary expressions that start with `[`. For function type constructors,
  /// we have to brace-match the closing paren and look for the `->`, which
  /// requires infinite lookahead.
  Syntax* Second_parser::parse_prefix_expression()
  {
    switch (lookahead())
    {
    case Token::lbracket_tok: {
      // Match array and template type constructors.
      Syntax* spec = parse_bracket_list();
      Token tok = match(Token::equal_greater_tok);
      Syntax* type = parse_prefix_expression();
      if (tok)
        return new Template_syntax(tok, spec, type);
      else
        return new Array_syntax(tok, spec, type);
    }

    case Token::lparen_tok: {
      // Match function type constructors.
      if (!starts_function_type(*this))
        break;
      Syntax* parms = parse_paren_list();
      Token tok = expect(Token::dash_greater_tok);
      Syntax* result = parse_prefix_expression();
      return new Function_syntax(tok, parms, result);
    }

    case Token::caret_tok:
    case Token::plus_tok:
    case Token::dash_tok:
    case Token::not_tok: {
      Token op = consume();
      Syntax* e = parse_prefix_expression();
      return new Prefix_syntax(op, e);
    }
    
    default:
      break;
    }
    return parse_postfix_expression();
  }

} // namespace beaker
