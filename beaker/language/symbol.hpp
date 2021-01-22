#ifndef BEAKER_LANGUAGE_SYMBOL_HPP
#define BEAKER_LANGUAGE_SYMBOL_HPP

#include <cassert>
#include <iosfwd>
#include <functional>
#include <string>
#include <unordered_set>

namespace beaker
{
  /// A unique string.
  struct Symbol
  {
    Symbol()
      : m_str()
    {
    }

    Symbol(std::string const* str)
      : m_str(str)
    {
    }

    /// Returns true if the symbol is valid.
    bool is_valid() const
    {
      return m_str;
    }

    /// Returns the length of the symbol.
    std::size_t size() const
    {
      assert(is_valid());
      return m_str->size();
    }

    /// Returns the underlying string.
    std::string const& str() const
    {
      assert(is_valid());
      return *m_str;
    }

    /// Returns the underlying c-string.
    char const* data() const
    {
      assert(is_valid());
      return m_str->data();
    }

    /// Returns true if and only if a and b are the same symbol.
    friend bool operator==(Symbol a, Symbol b)
    {
      return a.m_str == b.m_str;
    }

    /// Returns true if and only if a and b are different symbols.
    friend bool operator!=(Symbol a, Symbol b)
    {
      return a.m_str != b.m_str;
    }

    std::string const* m_str;
  };

  // Streaming

  template<typename C, typename T>
  std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>& os, Symbol s)
  {
    return os << s.str();
  }

  /// The symbol table constructs symbols.
  struct Symbol_table
  {
    Symbol get(char const* str)
    {
      return Symbol(&*m_strs.insert(str).first);
    }

    Symbol get(char const* first, char const* last)
    {
      return get(std::string(first, last));
    }

    Symbol get(char const* str, std::size_t n)
    {
      return get(std::string(str, n));
    }

    Symbol get(std::string const& str)
    {
      return Symbol(&*m_strs.insert(str).first);
    }

    std::unordered_set<std::string> m_strs;
  };

} // namespace beaker

// Standard hashing

namespace std
{
  template<>
  struct hash<beaker::Symbol>
  {
    std::size_t operator()(beaker::Symbol s) const noexcept
    {
      hash<std::string> h;
      return h(s.str());
    }
  };
} // namespace std

#endif
