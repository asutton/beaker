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

    diagnose_expected("declaration");
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
  ///     prefix-expression
  void Parser::parse_type()
  {
    return parse_prefix_expression();
  }

  /// Parse an expression.
  ///
  ///   expression:
  ///     impliciation-expression
  ///
  /// FIXME: Where do conditional expressions fit in? Lower or higher than
  /// implication? Or at the same precedence?
  void Parser::parse_expression()
  {
    return parse_implication_expression();
  }

  /// Parse an implication.
  ///
  ///   implication-expression:
  ///     logical-or-expression
  ///     logical-or-expression -> implication-expression
  void Parser::parse_implication_expression()
  {
    parse_prefix_expression();
    if (match(Token::arrow_tok))
      parse_implication_expression();
  }

  /// Parse an logical or.
  ///
  ///   logical-or-expression:
  ///     logical-and-expression
  ///     logical-or-expression or logical-and-expression
  void Parser::parse_logical_or_expression()
  {
    parse_logical_and_expression();
    while (match(Token::or_tok))
      parse_logical_and_expression();
  }

  /// Parse an logical and.
  ///
  ///   logical-and-expression:
  ///     equality-expression
  ///     logical-and-expression and equality-expression
  void Parser::parse_logical_and_expression()
  {
    parse_equality_expression();
    while (match(Token::and_tok))
      parse_equality_expression();
  }

  static bool is_equality_operator(Token::Kind k)
  {
    return k == Token::equal_equal_tok ||
           k == Token::bang_equal_tok;
  }

  /// Parse an equality comparison.
  ///
  ///   equality-expression:
  ///     relational-expression
  ///     equality-expression == relational-expression
  ///     equality-expression != relational-expression
  void Parser::parse_equality_expression()
  {
    parse_relational_expression();
    while (match_if(is_equality_operator))
      parse_relational_expression();
  }

  static bool is_relational_operator(Token::Kind k)
  {
    return k == Token::less_tok ||
           k == Token::greater_tok ||
           k == Token::less_equal_tok ||
           k == Token::greater_equal_tok;
  }

  /// Parse a relational expression.
  ///
  ///   relational-expression:
  ///     additive-expression
  ///     relational-expression < additive-expression
  ///     relational-expression > additive-expression
  ///     relational-expression <= additive-expression
  ///     relational-expression >= additive-expression
  void Parser::parse_relational_expression()
  {
    parse_additive_expression();
    while (match_if(is_relational_operator))
      parse_additive_expression();
  }

  static bool is_additive_operator(Token::Kind k)
  {
    return k == Token::plus_tok ||
           k == Token::dash_tok;
  }

  /// Parse an additive expression.
  ///
  ///   additive-expression:
  ///     multiplicative-expression
  ///     additive-expression < multiplicative-expression
  ///     additive-expression > multiplicative-expression
  void Parser::parse_additive_expression()
  {
    parse_multiplicative_expression();
    while (match_if(is_additive_operator))
      parse_multiplicative_expression();
  }

  static bool is_multiplicative_operator(Token::Kind k)
  {
    return k == Token::star_tok ||
           k == Token::slash_tok ||
           k == Token::percent_tok;
  }

  /// Parse a multiplicative expression.
  ///
  ///   multiplicative-expression:
  ///     prefix-exprssion
  ///     multiplicative-expression < prefix-exprssion
  ///     multiplicative-expression > prefix-exprssion
  void Parser::parse_multiplicative_expression()
  {
    parse_prefix_expression();
    while (match_if(is_multiplicative_operator))
      parse_prefix_expression();
  }

  /// Parse a prefix-expression.
  ///
  ///   prefix-expression:
  ///     postfix-expression
  ///     array [ expression-list? ] prefix-expression
  ///     templ [ expression-list? ] prefix-expression
  ///     func ( expression-list? ) prefix-expression
  ///     const prefix-exprssion
  ///     ^ prefix-expression
  ///     - prefix-expression
  ///     + prefix-expression
  ///     not prefix-expression
  ///
  /// NOTE: This is the minimal version of a grammar that both avoids extra
  /// lookahead and permits expressions and types to occupy the same grammar.
  /// We could add extra annotations after template and function type
  /// constructors (e.g., `func(int)->int`), but they aren't strictly necessary.
  ///
  /// TODO: The name array is somewhat unfortunate, since it makes a nice
  /// library structure. If arrays in this (or whatever) language had regular
  /// semantics, we probably wouldn't need the data type.
  ///
  /// Eliminating the leading keyword and adding an annotation before the prefix
  /// expression also works (e.g., (int)->int), but requires lookahead to
  /// disambiguate the enclosure from primary expressions. It also means that
  /// we can't parse implications, unless we had some way of explicitly
  /// characterizing the leading parameter list as something other than a
  /// primary expression.
  void Parser::parse_prefix_expression()
  {
    switch (lookahead())
    {
    case Token::array_tok:
      consume();
      parse_bracket_list();
      return parse_prefix_expression();

    case Token::templ_tok:
      consume();
      parse_bracket_list();
      return parse_prefix_expression();

    case Token::func_tok:
      consume();
      parse_paren_list();
      return parse_prefix_expression();

    case Token::caret_tok:
    case Token::plus_tok:
    case Token::dash_tok:
    case Token::not_tok:
      consume();
      return parse_prefix_expression();
    
    default:
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
  ///     postfix-expression .
  ///     postfix-expression ^
  void Parser::parse_postfix_expression()
  {
    parse_primary_expression();
    while (true)
    {
      if (next_token_is(Token::lparen_tok))
        parse_paren_list();
      else if (next_token_is(Token::lbracket_tok))
        parse_bracket_list();
      else if (match(Token::dot_tok))
        match(Token::identifier_tok);
      else if (match(Token::caret_tok))
        ;
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

    diagnose_expected("primary-expression");
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
      parse_expression_group();
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
      parse_expression_group();
    match(Token::rbracket_tok);
  }

  /// Parse an expression-group.
  ///
  ///   expression-group:
  ///     expression-list
  ///     expression-group ; expression-list
  void Parser::parse_expression_group()
  {
    parse_expression_list();
    while (match(Token::semicolon_tok))
      parse_expression_group();
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

  void Parser::diagnose_expected(char const* what)
  {
    std::stringstream ss;
    ss << input_location() << ": "
       << "expected '" << what 
       << "' but got '" << peek().spelling() << "'";
    throw std::runtime_error(ss.str());
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
