#ifndef BEAKER_TOOLS_FUZZER_FUZZER_HPP
#define BEAKER_TOOLS_FUZZER_FUZZER_HPP

#include <beaker/language/translation.hpp>
#include <beaker/frontend/token.hpp>
#include <beaker/frontend/syntax.hpp>

#include <random>

namespace beaker
{
  /// Randomly generates syntactically valid syntax.
  ///
  /// FIXME: Should this go in the frontend?
  struct Fuzzer
  {
    Fuzzer(Translation& trans);

    // Top-level.
    Syntax* fuzz_file();

    // Declarations.
    Syntax* fuzz_declaration();
    Syntax* fuzz_declaration_seq();
    Syntax* fuzz_definition();

    // Declarators.
    Syntax* fuzz_declarator();
    Syntax* fuzz_declarator_list();

    // Descriptor.
    Syntax* fuzz_descriptor();
    Syntax* fuzz_mapping_descriptor();

    // Constraints.
    Syntax* fuzz_constraint();

    // Expressions, in general.
    Syntax* fuzz_expression();

    // Cotrol expressions.
    Syntax* fuzz_leave_expression();
    Syntax* fuzz_control_expression();
    Syntax* fuzz_conditional_expression();
    Syntax* fuzz_match_expression();
    Syntax* fuzz_case_list();
    Syntax* fuzz_case();
    Syntax* fuzz_pattern_list();
    Syntax* fuzz_pattern();
    Syntax* fuzz_loop_expression();
    Syntax* fuzz_for_expression();
    Syntax* fuzz_while_expression();
    Syntax* fuzz_do_expression();
    Syntax* fuzz_lambda_expression();
    Syntax* fuzz_capture();
    Syntax* fuzz_let_expression();
    Syntax* fuzz_block_expression();
    Syntax* fuzz_block();

    // Infix expressions.
    Syntax* fuzz_assignment_expression();
    Syntax* fuzz_implication_expression();
    Syntax* fuzz_logical_or_expression();
    Syntax* fuzz_logical_and_expression();
    Syntax* fuzz_equality_expression();
    Syntax* fuzz_relational_expression();
    Syntax* fuzz_additive_expression();
    Syntax* fuzz_multiplicative_expression();
    
    // Prefix expressions.
    Syntax* fuzz_prefix_expression();
    Syntax* fuzz_template_constructor();
    Syntax* fuzz_array_constructor();
    Syntax* fuzz_function_constructor();
    
    // Postfix expressions.
    Syntax* fuzz_postfix_expression();
    
    // Primary expressions.
    Syntax* fuzz_primary_expression();
    Syntax* fuzz_id_expression();

    Syntax* fuzz_expression_list();

    // Parameters.    
    Syntax* fuzz_parameter_group();
    Syntax* fuzz_parameter_list();
    Syntax* fuzz_parameter();

    Syntax* fuzz_brace_list();

    // Statements
    Syntax* fuzz_statement();
    Syntax* fuzz_statement_seq();
    Syntax* fuzz_block_statement();
    Syntax* fuzz_declaration_statement();
    Syntax* fuzz_expression_statement();

    // Utilities

    enum class Enclosure
    {
      parens,
      brackets,
      braces,
    };

    struct Enclosing_tokens
    {
      Token::Kind open;
      Token::Kind close;
    };

    static constexpr Enclosing_tokens enclosing_toks[] = {
      { Token::lparen_tok, Token::rparen_tok },
      { Token::lbracket_tok, Token::rbracket_tok },
      { Token::lbrace_tok, Token::rbrace_tok },
    };

    static constexpr Token::Kind open_token(Enclosure e)
    {
      return enclosing_toks[(int)e].open;
    }

    static constexpr Token::Kind close_token(Enclosure e)
    {
      return enclosing_toks[(int)e].close;
    }

    template<Enclosure E, typename F>
    Syntax* fuzz_enclosed(F fn)
    {
      Syntax* s = (this->*fn)();
      Token o = make_token(open_token(E));
      Token c = make_token(close_token(E));
      return new Enclosure_syntax(o, c, s);
    }

    template<typename F>
    Syntax* fuzz_paren_enclosed(F fn)
    {
      return fuzz_enclosed<Enclosure::parens>(fn);
    }

    template<typename F>
    Syntax* fuzz_bracket_enclosed(F fn)
    {
      return fuzz_enclosed<Enclosure::brackets>(fn);
    }

    template<typename F>
    Syntax* fuzz_brace_enclosed(F fn)
    {
      return fuzz_enclosed<Enclosure::braces>(fn);
    }

    // Tokens

    /// Returns a token for `k`.
    Token make_token(Token::Kind k)
    {
      Symbol sym = m_trans.get_symbol(Token::spelling(k));
      return Token(k, sym);
    }

    Token make_where()
    {
      return make_token(Token::where_tok);
    }

    Token fuzz_literal();
    Token fuzz_identifier();

    // Random numbers

    /// Returns true or false with 50% probability.
    bool random_coin()
    {
      std::bernoulli_distribution rng;
      return rng(m_prng);
    }

    /// Returns a random number in `[0, n)`.
    int random_int(int n)
    {
      std::uniform_int_distribution<> rng(0, n - 1);
      return rng(m_prng);
    }

    Translation& m_trans;
    std::minstd_rand m_prng;
  };

} // namespace beaker

#endif
