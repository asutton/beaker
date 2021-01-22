#include <beaker/frontend/lexer.hpp>

#include <cctype>
#include <cstring>
#include <iostream>
#include <iterator>
#include <fstream>

namespace beaker
{
  static std::string read_file(std::filesystem::path const& p)
  {
    std::ifstream ifs(p);
    std::istreambuf_iterator<char> first(ifs);
    std::istreambuf_iterator<char> last;
    return std::string(first, last);
  }

  Lexer::Lexer(Translation& trans, std::filesystem::path const& p)
    : m_trans(trans), m_path(p), m_text(read_file(p)), m_pos(0), m_line_pos(0), m_line(1)
  {
    // Build the keyowrd table.
#define def_keyword(K) \
    m_keywords.emplace(m_trans.get_symbol(#K), Token::K ## _tok);
#include <beaker/frontend/token.def>

    // FIXME: If the input starts with a BOM mark, move past that. We also, need
    // to seriously adjust the lexing rules if we're in UTF-16 or UTF-32.
  }

  static void skip_space(Lexer& lex)
  {
    // Update line information.
    if (lex.m_text[lex.m_pos] == '\n') {
      lex.m_line_pos = lex.m_pos;
      ++lex.m_line;
    }
    ++lex.m_pos;
  }

  // TODO: Rewrite this as a scanner.
  static void skip_comment(Lexer& lex)
  {
    ++lex.m_pos;
    while (lex.m_pos < lex.m_text.size()) {
      if (lex.m_text[lex.m_pos] == '\n')
        return;
      ++lex.m_pos;
    }
  }

  template<typename S>
  static Token get_token(Lexer& lex)
  {
    char const* first = lex.m_text.data() + lex.m_pos;
    char const* last = lex.m_text.data() + lex.m_text.size();
    S scanner(lex, first, last);
    Token tok = scanner.get();
    lex.m_pos += tok.symbol().size();
    return tok;
  }

  static Token get_word(Lexer& lex)
  {
    return get_token<Word_scanner>(lex);
  }

  static Token get_number(Lexer& lex)
  {
    return get_token<Number_scanner>(lex);
  }

  static Token get_puncop(Lexer& lex)
  {
    return get_token<Puncop_scanner>(lex);
  }

  static bool is_identifier_start(char c)
  {
    return std::isalpha(c) || c == '_';
  }

  static bool is_identifier_rest(char c)
  {
    return std::isalpha(c) || std::isdigit(c) || c == '_';
  }

  Token Lexer::get()
  {
    while (m_pos < m_text.size()) {
      char c = m_text[m_pos];

      // Handle things that would be insignificant.
      if (std::isspace(c)) {
        skip_space(*this);
        continue;
      }
      else if (c == '#') {
        skip_comment(*this);
        continue;
      }

      // Handle things that are significant.
      if (is_identifier_start(c)) {
        return get_word(*this);
      }
      else if (std::isdigit(c)) {
        return get_number(*this);
      }
      else {
        return get_puncop(*this);
      }
    }
    return {};
  }

  Token Word_scanner::get()
  {
    char const* iter = m_first;
    ++iter;
    while (iter != m_last && is_identifier_rest(*iter))
      ++iter;
    Symbol sym = m_lex.m_trans.get_symbol(m_first, iter);
    Source_location loc = m_lex.input_location();

    // Match keywords
    auto kw = m_lex.m_keywords.find(sym);
    if (kw != m_lex.m_keywords.end())
      return Token(kw->second, sym, loc);

    return Token(Token::identifier_tok, sym, loc);
  }

  Token Number_scanner::get()
  {
    char const* iter = m_first;
    ++iter;
    while (iter != m_last && std::isdigit(*iter))
      ++iter;
    Symbol sym = m_lex.m_trans.get_symbol(m_first, iter);
    Source_location loc = m_lex.input_location();
    return Token(Token::identifier_tok, sym, loc);
  }

  // Returns
  static constexpr std::size_t known_token_length(Token::Kind k)
  {
    switch (k) {
#define def_singleton(K, S) \
    case Token::K ## _tok: \
      return std::strlen(S);
#include <beaker/frontend/token.def>
    default:
      break;
    }
    return -1;
  }

  template<Token::Kind K, std::size_t N>
  static Token get_singleton(Scanner& s)
  {
    static_assert(N != -1, "invalid token kind");
    Symbol sym = s.m_lex.m_trans.get_symbol(s.m_first, s.m_first + N);
    Source_location loc = s.m_lex.input_location();
    return Token(K, sym, loc);
  }


  template<Token::Kind K>
  static Token get_puncop(Scanner& s)
  {
    return get_singleton<K, known_token_length(K)>(s);
  }

  static Token get_invalid(Scanner& s)
  {
    return get_singleton<Token::invalid_tok, 1>(s);
  }

  Token Puncop_scanner::get()
  {
    switch (*m_first) {
    case '(':
      return get_puncop<Token::lparen_tok>(*this);
    case ')':
      return get_puncop<Token::rparen_tok>(*this);
    case '[':
      return get_puncop<Token::lbracket_tok>(*this);
    case ']':
      return get_puncop<Token::rbracket_tok>(*this);
    case '{':
      return get_puncop<Token::lbrace_tok>(*this);
    case '}':
      return get_puncop<Token::rbrace_tok>(*this);
    case ':':
      return get_puncop<Token::colon_tok>(*this);
    case ';':
      return get_puncop<Token::semicolon_tok>(*this);
    case ',':
      return get_puncop<Token::comma_tok>(*this);
    case '=':
      return get_puncop<Token::equal_tok>(*this);
    case '-':
      if (nth_char_is(1, '>'))
        return get_puncop<Token::arrow_tok>(*this);
      break;
    default:
      break;
    }
    return get_invalid(*this);
  }

} // namespace beaker
