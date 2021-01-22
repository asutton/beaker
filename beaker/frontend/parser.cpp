#include <beaker/frontend/parser.hpp>

#include <iostream>
#include <sstream>

namespace beaker
{
  Parser::Parser(Translation& trans, std::filesystem::path const& p)
    : m_trans(trans), m_lex(trans, p)
  {
    // Tokenize the input and point to the first token.
    m_lex.get(m_toks);
    m_pos = 0;
  }

  void Parser::parse_file()
  {
    parse_declaration_seq();
  }

  void Parser::parse_declaration_seq()
  {
    while (!eof())
      parse_declaration();
  }

  void Parser::parse_declaration()
  {
    switch (lookahead()) {
    case Token::def_tok:
      return parse_definition();
    default:
      break;
    }
    throw std::runtime_error("expected declaration");
  }

  /// Definition declaration:
  ///
  ///   definition-declaration:
  ///     def declarator : type ;
  ///     def declarator : = expression ;
  ///     def declarator : type = expression ;
  void Parser::parse_definition()
  {
    require(Token::def_tok);

    // Parse the declarator
    parse_declarator();

    // Parse the type.
    expect(Token::colon_tok);
    
    if (next_token_is_not(Token::equal_tok)) {
      parse_type();

      // Match the 'decl : type ;' case.
      if (match(Token::semicolon_tok))
        return;

      // Fall through to parse the initializer.
    }

    // Parse the initializer.
    expect(Token::equal_tok);
    parse_expression();
    
    expect(Token::semicolon_tok);
  }

  /// Parser a parameter:
  ///
  ///   identifier : type
  ///   identifier : type = expression
  ///   identifeir : = expression
  ///   : type
  ///   : type = expression
  void Parser::parse_parameter()
  {
    // Match unnamed variants.
    if (match(Token::colon_tok)) {
      parse_type();
      if (match(Token::equal_tok))
        parse_expression();
      return;
    }

    // Match the identifier...
    require(Token::identifier_tok);
    
    // ... And optional declarative information
    if (match(Token::colon_tok)) {
      if (next_token_is_not(Token::equal_tok))
        parse_type();
      if (match(Token::equal_tok))
        parse_expression();
    }
  }

  /// Parse a declarator.
  ///
  ///   declarator:
  ///     postfix-expression
  void Parser::parse_declarator()
  {
    return parse_postfix_expression();
  }

  /// Parse a type expression.
  ///
  ///   type-expression:
  ///     implication-expression
  void Parser::parse_type()
  {
    return parse_implication_expression();
  }

  void Parser::parse_expression()
  {
    return parse_implication_expression();
  }

  /// Parse an implication.
  ///
  ///   implication-expression:
  ///     prefix-expression
  ///     prefix-expression -> implication-expression
  void Parser::parse_implication_expression()
  {
    parse_prefix_expression();
    if (match(Token::arrow_tok))
      parse_implication_expression();
  }

  /// Parse a prefix-expression.
  ///
  ///   prefix-expression:
  ///     postfix-expression
  ///     [ expression-list? ] prefix-expression
  void Parser::parse_prefix_expression()
  {
    while (true) {
      if (next_token_is(Token::lbracket_tok))
        parse_bracket_list();
      else
        break;
    }
    return parse_postfix_expression();
  }

  /// Parse a postfix-expression.
  ///
  ///   postfix-expression:
  ///     primary-expression
  ///     postfix-expression ( expression-list )
  ///     postfix-expression [ expression-list ]
  void Parser::parse_postfix_expression()
  {
    parse_primary_expression();
    while (true)
    {
      if (next_token_is(Token::lparen_tok))
        parse_paren_list();
      else if (next_token_is(Token::lbracket_tok))
        parse_bracket_list();
      else
        return;
    }
  }

  /// Parse a primary expression.
  ///
  ///   primary-expression:
  ///     literal
  ///     identifier
  ///     ( expression-list? )
  ///     [ expression-list? ]
  ///     id-expression
  void Parser::parse_primary_expression()
  {
    switch (lookahead()) {
    case Token::true_tok:
    case Token::false_tok:
    case Token::integer_tok:
      // Value literals
      consume();
      return;

    case Token::int_tok:
    case Token::bool_tok:
    case Token::type_tok:
      // Type literals
      consume();
      return;

    case Token::identifier_tok:
      return parse_id_expression();

    case Token::lparen_tok:
      return parse_tuple_expression();

    case Token::lbracket_tok:
      return parse_list_expression();

    default:
      break;
    }

    // FIXME: Do better.
    std::stringstream ss;
    ss << input_location() 
       << ": expected a primary expression but got '"
       << peek().spelling() << "'";
    throw std::runtime_error(ss.str());
  }

  /// Parse a tuple-expression.
  ///
  ///   tuple-expression:
  ///     paren-list
  void Parser::parse_tuple_expression()
  {
    return parse_paren_list();
  }

  /// Parse a list-expression.
  ///
  ///   list-expression:
  ///     paren-list
  void Parser::parse_list_expression()
  {
    return parse_bracket_list();
  }

  /// Parse an id-expression
  ///
  ///   id-expression:
  ///     identifier
  void Parser::parse_id_expression()
  {
    require(Token::identifier_tok);
  }

  /// Parse a paren-enclosed list.
  ///
  ///   paren-list:
  ///     ( expression-list? )
  void Parser::parse_paren_list()
  {
    require(Token::lparen_tok);
    if (next_token_is_not (Token::rparen_tok))
      parse_expression_list();
    match(Token::rparen_tok);
  }

  /// Parse a bracket-enclosed list.
  ///
  ///   bracket-list:
  ///     [ expression-list? ]
  void Parser::parse_bracket_list()
  {
    require(Token::lbracket_tok);
    if (next_token_is_not (Token::rbracket_tok))
      parse_expression_list();
    match(Token::rbracket_tok);
  }

  /// Parse an expression-list.
  ///
  ///   expression-list:
  ///     parameter-expression
  ///     expression-list , parameter-expression
  ///
  /// TODO: Allow groupings of the form: `a, b : int; c : char`. Basically,
  /// just parse groups of things. These are ony valid when declaring
  /// parameters.
  void Parser::parse_expression_list()
  {
    parse_parameter_expression();
    while (match(Token::comma_tok))
      parse_parameter_expression();
  }

  static bool starts_parameter(Parser& p)
  {
    return p.next_token_is(Token::colon_tok) ||
           p.next_tokens_are(Token::identifier_tok, Token::colon_tok);
  }

  /// Parse a parameter or expression.
  ///
  ///   parameter-expression:
  ///     parameter
  ///     expression
  void Parser::parse_parameter_expression()
  {
    if (starts_parameter(*this))
      return parse_parameter();
    parse_expression();
  }

  void Parser::diagnose_expected(Token::Kind k)
  {
    std::stringstream ss;
    ss << input_location() << ": " 
       << "expected '" << Token::spelling(k) 
       << "' but got got '" << peek().spelling() << "'";
    throw std::runtime_error(ss.str());
  }

  void Parser::debug(char const* msg)
  {
    std::cerr << msg << ": " << input_location() << ": " << peek() << '\n';   
  }

} // namespace beaker
