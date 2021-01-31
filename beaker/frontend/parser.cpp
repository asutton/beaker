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

  /// Parse a file.
  ///
  ///   file:
  ///     declaration-seq?
  Syntax* Parser::parse_file()
  {
    Syntax* decls = nullptr;
    if (!eof())
      decls = parse_declaration_seq();
    return new File_syntax(decls);
  }

  /// Parse a sequence of declarations.
  ///
  ///   declaration-seq
  ///     declaration
  ///     declaration-seq declaration
  Syntax* Parser::parse_declaration_seq()
  {
    Syntax_seq ss;
    while (!eof())
      parse_item(*this, &Parser::parse_declaration, ss);
    return new Sequence_syntax(std::move(ss));
  }

  /// Parse a declaration:
  ///
  ///   declaration:
  ///     definition
  Syntax* Parser::parse_declaration()
  {
    return parse_definition();
  }

  /// Returns true if the tokens at `la` start a definition.
  static bool starts_definition(Parser& p, std::size_t la)
  {
    // Check for unnamed definitions.
    if (p.nth_token_is(la, Token::colon_tok))
      return true;
    
    // Check for definitions of the form `x:` and `x,y,z:`
    if (p.nth_token_is(la, Token::identifier_tok)) {
      ++la;

      // Match a single declarator.
      if (p.nth_token_is(la, Token::colon_tok))
        return true;
      
      // Match multiple declarators. Basically, search through a comma-separated
      // list of identifiers and stop when we reach anything else. Note that
      // finding anying the like `x, 0` means we can short-circuit at the
      // 0. There's no way this can be a definition.
      while (p.nth_token_is(la, Token::comma_tok)) {
        ++la;
        if (p.nth_token_is(la, Token::identifier_tok))
          ++la;
        else
          return false;
      }

      return p.nth_token_is(la, Token::colon_tok);
    }

    return false;
  }

  /// Returns true if `p` starts a parameter declaration.
  static bool starts_definition(Parser& p)
  {
    return starts_definition(p, 0);
  }

  namespace
  {
    struct Descriptor_clause
    {
      Syntax* type = {};
      Syntax* cons = {};
    };

    // Parse a descriptor clause.
    //
    //   descriptor-clause: 
    //     : descriptor constraint? 
    //     : constraint? 
    //
    //   constraint:
    //     is pattern
    Descriptor_clause parse_descriptor_clause(Parser& p)
    {
      Descriptor_clause dc;
      p.expect(Token::colon_tok);
      if (p.next_token_is(Token::is_tok)) {
        dc.cons = p.parse_constraint();
      }
      else {
        dc.type = p.parse_descriptor();
        if (p.next_token_is(Token::is_tok))
          dc.cons = p.parse_constraint();
      }
      return dc;
    }

    struct Initializer_clause
    {
      Syntax* init = {};
    };

    //   initializer-clause: 
    //     ; 
    //     = expression ; 
    //     = block-statement
    Initializer_clause parse_initializer_clause(Parser& p)
    {
      Initializer_clause clause;
      p.expect(Token::equal_tok);
      if (p.next_token_is(Token::lbrace_tok)) {
        clause.init = p.parse_block_statement();
      }
      else {
        clause.init = p.parse_expression();
        p.expect(Token::semicolon_tok);
      }
      return clause;
    }
  } // namespace

  /// Definition declaration:
  ///
  ///   definition-declaration:
  ///     declarator-list? type-clause initializer-clause 
  ///
  ///   type-clause: 
  ///     : type constraint? 
  ///     : constraint? 
  ///
  ///   type: 
  ///     prefix-expression 
  ///
  ///   constraint:
  ///     is pattern
  ///
  ///   initializer-clause: 
  ///     ; 
  ///     = expression ; 
  ///     = block-statement
  ///
  ///   block:
  ///     block-statement 
  Syntax* Parser::parse_definition()
  {
    // Parse the declarator-list.
    Syntax* decl = nullptr;
    if (next_token_is_not(Token::colon_tok))
      decl = parse_declarator_list();

    Descriptor_clause dc = parse_descriptor_clause(*this);
    Initializer_clause ic = parse_initializer_clause(*this);

    return new Declaration_syntax(decl, dc.type, dc.cons, ic.init);
  }

  /// Builds the declarator list.
  static Syntax* make_declarator_list(Syntax_seq& ts)
  {
    // TODO: What if `ts` is empty? Recovery means skipping the entire
    // declaration, probably.
    assert(!ts.empty());

    // Collapse singleton lists into simple declarators.
    if (ts.size() == 1)
      return ts[0];

    return new List_syntax(std::move(ts));
  }

  /// Parse a declarator-list.
  ///   declarator-list:
  ///     declarator
  ///     declartor-list , declarator
  ///
  /// Technically, this allows the declaration of multiple functions having
  /// the same return type, but we can semantically limit declarators to just
  /// variables.
  Syntax* Parser::parse_declarator_list()
  {
    Syntax_seq ts;
    parse_item(*this, &Parser::parse_declarator, ts);
    while (match(Token::comma_tok))
      parse_item(*this, &Parser::parse_declarator, ts);
    return make_declarator_list(ts);
  }

  /// Parse a declarator.
  ///
  ///   declarator:
  ///     id-expression
  ///
  /// This is a restriction on a more general grammar that allows function
  /// and/or array-like declarators.
  Syntax* Parser::parse_declarator()
  {
    return parse_id_expression();
  }

  /// Parse a type expression.
  ///
  ///   descriptor:
  ///     prefix-expression
  ///
  /// A descriptor specifies part of the signature of a declaration, including
  /// its template parameters, function parameters, array bounds, and type.
  Syntax* Parser::parse_descriptor()
  {
    return parse_prefix_expression();
  }

  /// Parse a mapping descriptor.
  ///
  ///   mapping-descriptor:
  ///     [ parameter-group ] prefix-expression
  ///     ( parameter-group ) prefix-expression
  Syntax* Parser::parse_mapping_descriptor()
  {
    assert(next_token_in({Token::lparen_tok, Token::lbracket_tok}));
    return parse_prefix_expression();
  }


  /// Parse a constraint.
  ///
  ///   constraint-clause:
  ///     is pattern
  ///
  /// TODO: We can have is constraints and where constraints. The difference
  /// is that in an is constraint, the declared entity implicitly participates
  /// in the expression, but not in a where constraint.
  Syntax* Parser::parse_constraint()
  {
    require(Token::is_tok);
    return parse_pattern();
  }

  /// Parse an expression.
  ///
  ///   expression:
  ///     leave-expression 
  ///     expression where ( parameter-group )
  Syntax* Parser::parse_expression()
  {
    Syntax* e0 = parse_leave_expression();
    while (Token op = match(Token::where_tok)) {
      Syntax* e1 = parse_paren_enclosed(&Parser::parse_parameter_group);
      new Infix_syntax(op, e0, e1);
    }
    return e0;
  }

  /// Parse a leave-expression.
  ///
  ///   leave-expression:
  ///     control-expression
  ///     return control-expression
  ///     throw control-expression
  Syntax* Parser::parse_leave_expression()
  {
    switch (lookahead()) {
    case Token::return_tok:
    case Token::throw_tok: {
      Token kw = consume();
      Syntax* s = parse_control_expression();
      return new Prefix_syntax(kw, s);
    }
    default:
      break;
    }
    return parse_control_expression();
  }

  /// Parse a control-expression.
  ///
  ///   control-expression:
  ///     assignment-expression
  ///     conditional-expression
  ///     case-expression
  ///     loop-expression
  ///     lambda-expression
  ///     let-expression
  Syntax* Parser::parse_control_expression()
  {
    switch (lookahead()) {
    case Token::if_tok:
      return parse_conditional_expression();
    case Token::case_tok:
    case Token::switch_tok:
      return parse_match_expression();
    case Token::for_tok:
    case Token::while_tok:
    case Token::do_tok:
      return parse_loop_expression();
    case Token::lambda_tok:
      return parse_lambda_expression();
    case Token::let_tok:
      return parse_let_expression();
    default:
      break;
    }
    return parse_assignment_expression();
  }

  /// Parse a conditional-expression.
  ///
  ///   conditional-expression:
  ///     if ( expression ) block-expression
  ///     if ( expression ) block-expression else block-expression
  Syntax* Parser::parse_conditional_expression()
  {
    Token ctrl = require(Token::if_tok);
    expect(Token::lparen_tok);
    Syntax* s0 = parse_expression();
    expect(Token::rparen_tok);
    Syntax* s1 = parse_block_expression();
    
    // Match the else part. Turn the body into a pair.
    if (match(Token::else_tok)) {
      Syntax* s2 = parse_block_expression();
      s1 = new Pair_syntax(s1, s2);
    }
    return new Control_syntax(ctrl, s0, s1);
  }

  /// Parse a match-expression.
  ///
  ///   match-expression:
  ///     case ( expression ) case-list
  ///     switch ( expression ) case-list
  Syntax* Parser::parse_match_expression()
  {
    assert(next_token_is(Token::case_tok) || next_token_is(Token::switch_tok));
    Token ctrl = consume();
    expect(Token::lparen_tok);
    Syntax* s0 = parse_expression();
    expect(Token::rparen_tok);
    Syntax* s1 = parse_case_list();
    return new Control_syntax(ctrl, s0, s1);
  }

  /// Parses a case-list.
  ///
  ///   case-list:
  ///     case
  ///     case-list else case
  Syntax* Parser::parse_case_list()
  {
    std::vector<Syntax*> cs;
    parse_item(*this, &Parser::parse_case, cs);
    while (match(Token::else_tok))
      parse_item(*this, &Parser::parse_case, cs);
    return new List_syntax(std::move(cs));
  }

  /// Parse a case in a match-expression:
  ///
  ///   case:
  ///     pattern-list? => block-expression
  Syntax* Parser::parse_case()
  {
    Syntax* s0 = nullptr;
    if (next_token_is_not(Token::equal_greater_tok))
      s0 = parse_pattern_list();
    Token op = expect(Token::equal_greater_tok);
    Syntax* s1 = parse_block_expression();
    return new Infix_syntax(op, s0, s1);
  }

  /// Parse a pattern-list.
  ///
  ///   pattern-list:
  ///     pattern
  ///     pattern-list , pattern
  Syntax* Parser::parse_pattern_list()
  {
    std::vector<Syntax*> ps;
    parse_item(*this, &Parser::parse_pattern, ps);
    while (match(Token::comma_tok))
      parse_item(*this, &Parser::parse_pattern, ps);
    return new List_syntax(std::move(ps));
  }

  /// Parse a pattern.
  ///
  ///   pattern:
  ///     prefix-expression
  ///
  /// A pattern describes a set of types or values.
  Syntax* Parser::parse_pattern()
  {
    return parse_prefix_expression();
  }

  /// Parse a loop expression.
  ///
  ///   loop-expression:
  ///     for ( declarator type-clause in expression ) block-expression
  ///     while ( expression ) block-expression
  ///     do block-expression while ( condition )
  ///     do block-expression
  Syntax* Parser::parse_loop_expression()
  {
    switch (lookahead()) {
    case Token::for_tok:
    case Token::while_tok:
    case Token::do_tok:
    default:
      break;
    }
    assert(false);
  }

  /// Parse a for loop.
  ///
  ///   loop-expression:
  ///     for ( declarator descriptor-clause in expression ) block-expression
  ///
  /// The declaration in the parameter list is similar to normal parameters
  /// except that the `=` is replaced by `in`.
  ///
  /// TODO: Can we generalize the syntax for multiple parameters? What would
  /// it mean? There are a few options (zip vs. cross). Note that these
  /// don't group like other parameters.
  Syntax* Parser::parse_for_expression()
  {
    Token ctrl = require(Token::for_tok);
    expect(Token::lparen_tok);
    Syntax* id = parse_declarator();
    Descriptor_clause dc = parse_descriptor_clause(*this);
    expect(Token::in_tok);
    Syntax* init = parse_expression();
    Syntax* decl = new Declaration_syntax(id, dc.type, dc.cons, init);
    expect(Token::rparen_tok);
    Syntax* body = parse_block_expression();
    return new Control_syntax(ctrl, decl, body);
  }

  ///   loop-expression:
  ///     while ( expression ) block-expression
  Syntax* Parser::parse_while_expression()
  {
    Token ctrl = require(Token::while_tok);
    expect(Token::lparen_tok);
    Syntax* s0 = parse_expression();
    expect(Token::rparen_tok);
    Syntax* s1 = parse_block_expression();
    return new Control_syntax(ctrl, s0, s1);
  }

  /// Parse a do/do-while expression.
  ///
  ///   loop-expression:
  ///     do block-expression while ( condition )
  ///     do block-expression
  ///
  /// Note that the "head" of the do expression appears after the body
  /// of the loop (if it appears at all).
  Syntax* Parser::parse_do_expression()
  {
    Token ctrl = require(Token::do_tok);
    Syntax* s1 = parse_block_expression();
    Syntax* s0 = nullptr;
    if (match(Token::while_tok)) {
      expect(Token::lparen_tok);
      s0 = parse_expression();
      expect(Token::rparen_tok);
    }
    return new Control_syntax(ctrl, s0, s1);
  }

  static bool is_open_token(Token::Kind k)
  {
    switch (k) {
    case Token::lparen_tok:
    case Token::lbracket_tok:
    case Token::lbrace_tok:
      return true;
    default:
      return false;
    }
  }

  static bool is_close_token(Token::Kind k)
  {
    switch (k) {
    case Token::rparen_tok:
    case Token::rbracket_tok:
    case Token::rbrace_tok:
      return true;
    default:
      return false;
    }
  }

  /// Find the matching offset of the current token.
  static std::size_t find_matched(Parser& p, Token::Kind open, Token::Kind close)
  {
    assert(p.lookahead() == open);
    std::size_t la = 0;
    std::size_t braces = 0;
    while (Token::Kind k = p.lookahead(la)) {
      if (is_open_token(k)) {
        ++braces;
      }
      else if (is_close_token(k)) {
        --braces;
        if (braces == 0)
          break;
      }
      ++la;
    }
    return la;
  }

  // Returns true if the current sequnce of tokens a capture block.
  static bool is_capture(Parser& p)
  {
    assert(p.next_token_is(Token::lbrace_tok));
    std::size_t la = find_matched(p, Token::lbrace_tok, Token::rbrace_tok);
    switch (p.lookahead(la + 1)) {
    case Token::lparen_tok:
    case Token::lbracket_tok:
    case Token::is_tok:
    case Token::equal_greater_tok:
      return true;
    default:
      break;
    }
    return false;
  }

  /// Parse a lambda-expression.
  ///
  ///   lambda-expression:
  ///     lambda capture? mapping-descriptor constraint? => block-expression
  ///     lambda block-expression
  ///
  /// The capture, descriptor, and constraint comprise the head and are
  /// stored in a triple.
  Syntax* Parser::parse_lambda_expression()
  {
    Token ctrl = require(Token::lambda_tok);

    Syntax* cap = nullptr;
    if (next_token_is(Token::lbrace_tok))
    {
      if (!is_capture(*this)) {
        Syntax* body = parse_block_expression();
        return new Control_syntax(ctrl, nullptr, body);
      }
      cap = parse_capture();
    }
    
    Syntax* desc = nullptr;
    if (next_token_in({Token::lparen_tok, Token::lbracket_tok}))
      desc = parse_mapping_descriptor();

    Syntax* cons = nullptr;
    if (next_token_is(Token::is_tok))
      cons = parse_constraint();

    expect(Token::equal_greater_tok);
    Syntax* body = parse_block_expression();

    Syntax* head = new Triple_syntax(cap, desc, cons);
    return new Control_syntax(ctrl, head, body);
  }

  /// Parse a lambda capture.
  ///
  ///   capture:
  ///     block-statement
  Syntax* Parser::parse_capture()
  {
    return parse_block_statement();
  }

  /// Parse a let expression.
  ///
  ///   let-expression:
  ///     let ( parameter-group ) block-or-expression
  Syntax* Parser::parse_let_expression()
  {
    Token ctrl = require(Token::let_tok);
    Syntax* head = parse_paren_enclosed(&Parser::parse_parameter_group);
    Syntax* body = parse_block_expression();
    return new Control_syntax(ctrl, head, body);
  }

  /// Parse a block-expression.
  ///
  ///   block-expression:
  ///     expression
  ///     block
  Syntax* Parser::parse_block_expression()
  {
    if (next_token_is(Token::lbrace_tok))
      return parse_block();
    return parse_expression();
  }

  /// Parse a block.
  ///
  ///   block:
  ///     block-statement
  Syntax* Parser::parse_block()
  {
    return parse_block_statement();
  }

  /// Parse an assignment.
  ///
  ///   assignment-expression:
  ///     implication-expression
  ///     implication-expression = assignment-expression
  Syntax* Parser::parse_assignment_expression()
  {
    Syntax* e0 = parse_implication_expression();
    if (Token op = match(Token::equal_tok)) {
      Syntax* e1 = parse_assignment_expression();
      return new Infix_syntax(op, e0, e1);
    }
    return e0;
  }

  /// Parse an implication.
  ///
  ///   implication-expression:
  ///     logical-or-expression
  ///     logical-or-expression -> implication-expression
  Syntax* Parser::parse_implication_expression()
  {
    Syntax* e0 = parse_logical_or_expression();
    if (Token op = match(Token::dash_greater_tok)) {
      Syntax* e1 = parse_implication_expression();
      return new Infix_syntax(op, e0, e1);
    }
    return e0;
  }

  /// Parse an logical or.
  ///
  ///   logical-or-expression:
  ///     logical-and-expression
  ///     logical-or-expression or logical-and-expression
  Syntax* Parser::parse_logical_or_expression()
  {
    Syntax* e0 = parse_logical_and_expression();
    while (Token op = match(Token::or_tok)) {
      Syntax* e1 = parse_logical_and_expression();
      e0 = new Infix_syntax(op, e0, e1);
    }
    return e0;
  }

  /// Parse an logical and.
  ///
  ///   logical-and-expression:
  ///     equality-expression
  ///     logical-and-expression and equality-expression
  Syntax* Parser::parse_logical_and_expression()
  {
    Syntax* e0 = parse_equality_expression();
    while (Token op = match(Token::and_tok)) {
      Syntax* e1 = parse_equality_expression();
      e0 = new Infix_syntax(op, e0, e1);
    }
    return e0;
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
  Syntax* Parser::parse_equality_expression()
  {
    Syntax* e0 = parse_relational_expression();
    while (Token op = match_if(is_equality_operator)) {
      Syntax* e1 = parse_relational_expression();
      e0 = new Infix_syntax(op, e0, e1);
    }
    return e0;
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
  Syntax* Parser::parse_relational_expression()
  {
    Syntax* e0 = parse_additive_expression();
    while (Token op = match_if(is_relational_operator)) {
      Syntax* e1 = parse_additive_expression();
      e0 = new Infix_syntax(op, e0, e1);
    }
    return e0;
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
  ///     additive-expression + multiplicative-expression
  ///     additive-expression - multiplicative-expression
  Syntax* Parser::parse_additive_expression()
  {
    Syntax* e0 = parse_multiplicative_expression();
    while (Token op = match_if(is_additive_operator)) {
      Syntax* e1 = parse_multiplicative_expression();
      e0 = new Infix_syntax(op, e0, e1);
    }
    return e0;
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
  ///     prefix-expression
  ///     multiplicative-expression * prefix-expression
  ///     multiplicative-expression / prefix-expression
  ///     multiplicative-expression % prefix-expression
  Syntax* Parser::parse_multiplicative_expression()
  {
    Syntax* e0 = parse_prefix_expression();
    while (Token op = match_if(is_multiplicative_operator)) {
      Syntax* e1 = parse_prefix_expression();
      e0 = new Infix_syntax(op, e0, e1);
    }
    return e0;
  }

  // An lparen starts a prefix operator if the first few tokens start a
  // prefix operator, and the entire enclosure is not followed by something
  // that is eithe a primary expression or other prefix operator.
  static bool is_prefix_operator(Parser& p, Token::Kind open, Token::Kind close)
  {
    std::size_t la = find_matched(p, open, close);
    switch (p.lookahead(la + 1)) {
      // Primary expressions.
      case Token::true_tok:
      case Token::false_tok:
      case Token::integer_tok:
      case Token::int_tok:
      case Token::bool_tok:
      case Token::type_tok:
      case Token::identifier_tok:
      // Both prefix and primary.
      case Token::lparen_tok:
      // Other prefix operators.
      case Token::lbracket_tok:
      case Token::const_tok:
      case Token::caret_tok:
      case Token::plus_tok:
      case Token::dash_tok:
      case Token::not_tok:
        return true;
      default:
        break;
    }
    return false;
  }

  /// Contains information about the lexical structure of a potential prefix
  /// operator.
  struct Enclosure_characterization
  {
    /// True if we have `()` or `[]`.
    bool is_empty = false;

    /// True if the non-empty contents are `:t`, `x:t`, or `x0, ..., xn:t`.
    bool has_parameters = false;
    
    /// True if the token following the closing `)` or `]` starts a prefix
    /// or primary expression.
    bool is_operator = false;
  };

  // For a term like `@ parameter-group | expression-list @`, determine some
  // essential properties.
  Enclosure_characterization characterize_prefix_op(Parser& p)
  {
    assert(p.next_token_in({Token::lparen_tok, Token::lbracket_tok}));

    // Get the matching token kinds.
    Token::Kind open = p.lookahead();
    Token::Kind close = open == Token::lparen_tok
      ? Token::rparen_tok 
      : Token::rbracket_tok;
    
    Enclosure_characterization info;

    // Characterize the enclosure.
    if (p.nth_token_is(1, close))
      info.is_empty = true;
    else if (starts_definition(p, 1))
      info.has_parameters = true;
    
    // Characterize the token after the enclosure.
    info.is_operator = is_prefix_operator(p, open, close);

    return info;
  }

  /// Parse a prefix-expression.
  ///
  ///   prefix-expression:
  ///     postfix-expression
  ///     [ expression-list? ] prefix-expression
  ///     [ parameter-group ] prefix-expression
  ///     ( parameter-group? ) prefix-expression
  ///     const prefix-expression
  ///     ^ prefix-expression
  ///     - prefix-expression
  ///     + prefix-expression
  ///     not prefix-expression
  Syntax* Parser::parse_prefix_expression()
  {
    switch (lookahead())
    {
    case Token::lbracket_tok: {
      auto info = characterize_prefix_op(*this);
      if (!info.is_operator)
        break;
      if (!info.is_empty && info.has_parameters)
        return parse_template_constructor();
      else
        return parse_array_constructor();
    }

    case Token::lparen_tok: {
      auto info = characterize_prefix_op(*this);
      if (!info.is_operator)
        break;
      return parse_function_constructor();
    }

    case Token::const_tok:
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

  /// Parse a template type constructor.
  ///
  ///   prefix-expression:
  ///     [ parameter-group ] prefix-expression
  Syntax* Parser::parse_template_constructor()
  {
    Syntax* op = parse_bracket_enclosed(&Parser::parse_parameter_group);
    Syntax* type = parse_prefix_expression();
    return new Template_syntax(op, type);
  }

  /// Parse an array type constructor.
  ///
  ///   prefix-expression:
  ///     [ expression-list? ] prefix-expression
  Syntax* Parser::parse_array_constructor()
  {
    Syntax* op = parse_bracket_enclosed(&Parser::parse_expression_list);
    Syntax* type = parse_prefix_expression();
    return new Array_syntax(op, type);
  }

  /// Parse a template type constructor.
  ///
  ///   prefix-expression:
  ///     ( parameter-group? ) prefix-expression
  Syntax* Parser::parse_function_constructor()
  {
    Syntax* op = parse_paren_enclosed(&Parser::parse_parameter_group);
    Syntax* type = parse_prefix_expression();
    return new Template_syntax(op, type);
  }

  /// Parse a postfix-expression.
  ///
  ///   postfix-expression:
  ///     primary-expression
  ///     postfix-expression ( expression-list? )
  ///     postfix-expression [ expression-list? ]
  ///     postfix-expression . id-expression
  ///     postfix-expression ^
  Syntax* Parser::parse_postfix_expression()
  {
    Syntax* e0 = parse_primary_expression();
    while (true)
    {
      if (next_token_is(Token::lparen_tok)) {
        Syntax* args = parse_paren_enclosed(&Parser::parse_expression_list);
        e0 = new Call_syntax(e0, args);
      }
      else if (next_token_is(Token::lbracket_tok)) {
        Syntax* args = parse_bracket_enclosed(&Parser::parse_expression_list);
        e0 = new Call_syntax(e0, args);
      }
      else if (Token dot = match(Token::dot_tok)) {
        Syntax* member = parse_id_expression();
        e0 = new Infix_syntax(dot, e0, member);
      }
      else if (Token op = match(Token::caret_tok)) {
        e0 = new Postfix_syntax(op, e0);
      }        
      else
        break;
    }
    return e0;
  }

  // For a term like `@ parameter-group | expression-list @`, determine some
  // essential properties. This is basically the same as the characterize
  // function above, except that it doesn't look past the end of the
  // enclosure.
  Enclosure_characterization characterize_primary_expr(Parser& p)
  {
    assert(p.next_token_in({Token::lparen_tok, Token::lbracket_tok}));

    // Get the matching token kinds.
    Token::Kind open = p.lookahead();
    Token::Kind close = open == Token::lparen_tok
      ? Token::rparen_tok 
      : Token::rbracket_tok;

    Enclosure_characterization info;

    // Characterize the enclosure.
    if (p.nth_token_is(1, close))
      info.is_empty = true;
    else if (starts_definition(p, 1))
      info.has_parameters = true;

    return info;
  }

  /// Parse a primary expression.
  ///
  ///   primary-expression:
  ///     literal
  ///     id-expression
  ///     ( expression-list? )
  ///     ( parameter-group? )
  ///     [ parameter-group? ]
  ///
  /// The enclosed parameter groups exist to allow the omission of the result
  /// type of template and function type constructors.
  Syntax* Parser::parse_primary_expression()
  {
    switch (lookahead()) {
      // Value literals
    case Token::true_tok:
    case Token::false_tok:
    // Type literals
    case Token::int_tok:
    case Token::bool_tok:
    case Token::type_tok: 
    // Control primitives
    case Token::continue_tok:
    case Token::break_tok: {
      Token value = consume();
      return new Literal_syntax(value);
    }

    case Token::identifier_tok:
      return parse_id_expression();

    case Token::lparen_tok: {
      auto info = characterize_prefix_op(*this);
      if (!info.is_empty && info.has_parameters)
        return parse_paren_enclosed(&Parser::parse_parameter_group);
      return parse_paren_enclosed(&Parser::parse_expression_list);
    }

    case Token::lbracket_tok:
      return parse_bracket_enclosed(&Parser::parse_parameter_group);
    
    default:
      break;
    }

    // FIXME: Return an error tree. Also, how can we recover from this?
    // It might depend on what we're parsing (declarator, type, initialzer,
    // etc.). To do that, we'd have to maintain a stack of recovery strategies
    // that we can use to skip tokens.
    diagnose_expected("primary-expression");
    return nullptr;
  }

  /// Parse an id-expression
  ///
  ///   id-expression:
  ///     identifier
  Syntax* Parser::parse_id_expression()
  {
    Token id = expect(Token::identifier_tok);
    return new Identifier_syntax(id);
  }

  /// Parse an expression-list.
  ///
  ///   expression-list:
  ///     expression
  ///     expression-list , expression
  ///
  /// This always returns a list, even if there's a single element.
  Syntax* Parser::parse_expression_list()
  {
    Syntax_seq ts;
    parse_item(*this, &Parser::parse_expression, ts);
    while (match(Token::comma_tok))
      parse_item(*this, &Parser::parse_expression, ts);
    return new List_syntax(std::move(ts));
  }

  /// Returns a list defining the group.
  static Syntax* make_parameter_group(std::vector<Syntax*>& ts)
  {
    // This only happens when there's an error and we can't accumulate
    // a group. If we propagate errors, this shouldn't happen at all.
    if (ts.empty())
      return nullptr;    

    // Don't allocate groups if there's only one present.    
    if (ts.size() == 1)
      return ts[0];

    return new List_syntax(std::move(ts));
  }

  /// Parse an expression-group.
  ///
  ///   parameter-group:
  ///     parameter-list
  ///     parameter-group ; parameter-list
  ///
  /// Groups are only created if multiple groups are present.
  Syntax* Parser::parse_parameter_group()
  {
    std::vector<Syntax*> ts;
    parse_item(*this, &Parser::parse_parameter_list, ts);
    while (match(Token::semicolon_tok))
      parse_item(*this, &Parser::parse_parameter_list, ts);
    return make_parameter_group(ts);
  }

  // Returns a list for `ts`.
  static Syntax* make_parameter_list(std::vector<Syntax*>& ts)
  {
    // This only happens when an error occurred.
    if (ts.empty())
      return nullptr;

    return new List_syntax(std::move(ts));
  }

  /// Parse an parameter-list.
  ///
  ///   parameter-list:
  ///     parameter
  ///     parameter-list , parameter
  ///
  /// This always returns a list, even if there's a single element.
  Syntax* Parser::parse_parameter_list()
  {
    std::vector<Syntax*> ts;
    parse_item(*this, &Parser::parse_parameter, ts);
    while (match(Token::comma_tok))
      parse_item(*this, &Parser::parse_parameter, ts);
    return make_parameter_list(ts);
  }

  /// Parser a parameter:
  ///
  ///   parameter:
  ///     identifier : type
  ///     identifier : type = expression
  ///     identifeir : = expression
  ///     : type
  ///     : type = expression
  ///
  /// TODO: Can parameters have introducers?
  ///
  /// TODO: Can paramters be packs (yes, but what's the syntax?).
  Syntax* Parser::parse_parameter()
  {
    // Match unnamed variants.
    if (match(Token::colon_tok)) {
      Syntax* type = parse_descriptor();
      Syntax* init = nullptr;
      if (match(Token::equal_tok))
        init = parse_expression();
      return new Declaration_syntax(nullptr, type, nullptr, init);
    }

    // Match the identifier...
    Syntax* id = parse_id_expression();
    
    // ... And optional declarative information
    Syntax* type = nullptr;
    Syntax* init = nullptr;
    if (match(Token::colon_tok)) {
      if (next_token_is_not(Token::equal_tok))
        type = parse_descriptor();
      if (match(Token::equal_tok))
        init = parse_expression();
    }

    return new Declaration_syntax(id, type, nullptr, init);
  }

  /// Parse a brace-enclosed list.
  ///
  ///   brace-list:
  ///     { statement-seq }
  Syntax* Parser::parse_brace_list()
  {
    return parse_enclosed<Enclosure::braces>(&Parser::parse_statement_seq);
  }

  /// Parse a statement sequence.
  Syntax* Parser::parse_statement_seq()
  {
    Syntax_seq ts;
    parse_item(*this, &Parser::parse_statement, ts);
    while (next_token_is_not(Token::rbrace_tok))
      parse_item(*this, &Parser::parse_statement, ts);

    return make_declarator_list(ts);
  }

  /// Parse a statement.
  ///
  ///   statement:
  ///     block-statement
  ///     declaration-statement
  ///     expression-statement
  Syntax* Parser::parse_statement()
  {
    if (next_token_is(Token::lbrace_tok))
      return parse_block_statement();

    if (starts_definition(*this))
      return parse_declaration_statement();

    return parse_expression_statement();
  }

  /// Parse a block-statement.
  ///
  ///   block-statement:
  ///     { statement-seq }
  Syntax* Parser::parse_block_statement()
  {
    return parse_brace_enclosed(&Parser::parse_statement_seq);
  }

  /// Parse a declaration-statement.
  ///
  ///   declaration-statement:
  ///     declaration
  ///
  /// Not all declarations are allowed in all scopes. However, we don't
  /// really have a notion of scope attached to the parse, so we have to
  /// filter semantically.
  Syntax* Parser::parse_declaration_statement()
  {
    return parse_declaration();
  }

  /// Parse an expression-statement.
  ///
  ///   expression-statement:
  ///     expression-list ;
  Syntax* Parser::parse_expression_statement()
  {
    Syntax* e = parse_expression_list();
    expect(Token::semicolon_tok);
    return e;
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
    diagnose_expected(Token::spelling(k));
  }

  void Parser::debug(char const* msg)
  {
    std::cerr << msg << ": " << input_location() << ": " << peek() << '\n';   
  }

} // namespace beaker
