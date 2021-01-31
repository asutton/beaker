#include <beaker/tools/fuzzer/fuzzer.hpp>

namespace beaker
{
  Fuzzer::Fuzzer(Translation& trans)
    : m_trans(trans), m_prng(std::random_device{}())
  { }

  Syntax* Fuzzer::fuzz_file()
  {
    // FIXME: Test empty files with low probability.
    Syntax* ds = fuzz_declaration_seq();
    return new File_syntax(ds);
  }

  Syntax* Fuzzer::fuzz_declaration_seq()
  {
    // FIXME: How should we control how long this is?
    std::vector<Syntax*> ds;
    ds.push_back(fuzz_declaration());
    while (random_coin())
      ds.push_back(fuzz_declaration());
    return new Sequence_syntax(std::move(ds));
  }

  Syntax* Fuzzer::fuzz_declaration()
  {
    return fuzz_definition();
  }

  Syntax* Fuzzer::fuzz_definition()
  {
    Syntax* decl = fuzz_declarator_list();
    Syntax* desc = fuzz_descriptor();
    Syntax* cons = fuzz_constraint();
    Syntax* init = fuzz_expression();
    return new Declaration_syntax(decl, desc, cons, init);
  }

  static Syntax* make_declarator_list(std::vector<Syntax*>& ds)
  {
    if (ds.size() == 1)
      return ds[0];
    return new List_syntax(std::move(ds));
  }

  Syntax* Fuzzer::fuzz_declarator_list()
  {
    std::vector<Syntax*> ds;
    ds.push_back(fuzz_declarator());
    while (random_coin())
      ds.push_back(fuzz_declarator());
    return make_declarator_list(ds);
  }

  Syntax* Fuzzer::fuzz_declarator()
  {
    return fuzz_id_expression();
  }

  Syntax* Fuzzer::fuzz_descriptor()
  {
    return fuzz_prefix_expression();
  }

  Syntax* Fuzzer::fuzz_mapping_descriptor()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_constraint()
  {
    return fuzz_pattern();
  }

  Syntax* Fuzzer::fuzz_expression()
  {
    Syntax* s0 = fuzz_leave_expression();
    // if (random_coin()) {
    //   Token tok = make_where();
    //   Syntax* s1 = fuzz_paren_enclosed(&Fuzzer::fuzz_parameter_group);
    //   return new Infix_syntax(tok, s0, s1);
    // }
    return s0;
  }

  Syntax* Fuzzer::fuzz_leave_expression()
  {
    // FIXME: Wrong.
    return fuzz_primary_expression();
  }

  Syntax* Fuzzer::fuzz_control_expression()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_conditional_expression()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_match_expression()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_case_list()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_case()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_pattern_list()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_pattern()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_loop_expression()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_for_expression()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_while_expression()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_do_expression()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_lambda_expression()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_capture()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_let_expression()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_block_expression()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_block()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_assignment_expression()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_implication_expression()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_logical_or_expression()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_logical_and_expression()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_equality_expression()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_relational_expression()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_additive_expression()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_multiplicative_expression()
  {
    return nullptr;
  }
  
  Syntax* Fuzzer::fuzz_prefix_expression()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_template_constructor()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_array_constructor()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_function_constructor()
  {
    return nullptr;
  }
  
  Syntax* Fuzzer::fuzz_postfix_expression()
  {
    return nullptr;
  }
  
  Syntax* Fuzzer::fuzz_primary_expression()
  {
    // FIXME: paren and bracket expressions. Also, break and continue.
    switch (random_int(2)) {
    case 0:
      return new Literal_syntax(fuzz_literal());
    case 1:
      return fuzz_id_expression();
    case 2:
      return fuzz_paren_enclosed(&Fuzzer::fuzz_expression_list);
    }

    return nullptr;
  }

  Token Fuzzer::fuzz_literal()
  {
    Token toks[] {
      make_token(Token::true_tok),
      make_token(Token::false_tok),
      make_token(Token::bool_tok),
      make_token(Token::int_tok),
      make_token(Token::type_tok)
    };
    std::size_t n = sizeof(toks) / sizeof(Token);
    return toks[random_int(n)];
  }

  Syntax* Fuzzer::fuzz_id_expression()
  {
    return new Identifier_syntax(fuzz_identifier());
  }

  Syntax* Fuzzer::fuzz_expression_list()
  {
    // FIXME: Probably make empty lists rare.
    std::vector<Syntax*> es;
    while (random_coin())
      es.push_back(fuzz_expression());
    return new List_syntax(std::move(es));
  }

  Syntax* Fuzzer::fuzz_parameter_group()
  {
    // FIXME: Wrong.
    return fuzz_parameter_list();
  }

  Syntax* Fuzzer::fuzz_parameter_list()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_parameter()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_brace_list()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_statement()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_statement_seq()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_block_statement()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_declaration_statement()
  {
    return nullptr;
  }

  Syntax* Fuzzer::fuzz_expression_statement()
  {
    return nullptr;
  }

  Token Fuzzer::fuzz_identifier()
  {
    std::string str(random_int(5) + 1, ' ');
    for (char& c : str)
      c = 'a' + random_int(26);
    Symbol sym = m_trans.get_symbol(str);
    return Token(Token::identifier_tok, sym);
  }

} // namespace beaker