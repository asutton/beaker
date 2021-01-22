#include <beaker/frontend/token.hpp>

#include <iostream>

namespace beaker
{
  char const* Token::kind_name() const
  {
    switch (m_kind)
    {
#define def_token(K) \
    case K ## _tok: \
      return #K;
#include <beaker/frontend/token.def>
    default:
      break;
    }
    assert(false);
  }

  bool Token::is_singleton() const
  {
    switch (m_kind)
    {
#define def_token(K) \
    case K ## _tok: \
      return false;
#define def_singleton(K, S) \
    case K ## _tok: \
      return true;
#include <beaker/frontend/token.def>
    default:
      break;
    }
    assert(false);
  }

  char const* Token::spelling(Kind k)
  {
    switch (k)
    {
#define def_singleton(K, S) \
    case K ## _tok: \
      return S;
#include <beaker/frontend/token.def>
    default:
      break;
    }
    assert(false);
  }

  std::ostream& operator<<(std::ostream& os, Token const& tok)
  {
    os << '<';
    os << tok.kind_name();
    if (tok.is_class())
      os << ':' << tok.spelling();
    os << '>';
    return os;
  }

} // namespace beaker
