#include <beaker/frontend/syntax.hpp>

#include <iostream>
#include <string>

namespace beaker
{
  /// Returns the kind name.
  char const* Syntax::kind_name() const
  {
    switch (m_kind) {
#define def_syntax(T, K) \
    case K ## _kind: \
      return #K;
#include <beaker/frontend/syntax.def>
    default:
      break;
    }
    assert(false);
  }

  /// Returns the class name.
  char const* Syntax::class_name() const
  {
    switch (m_kind) {
#define def_syntax(T, K) \
    case K ## _kind: \
      return #T;
#include <beaker/frontend/syntax.def>
    default:
      break;
    }
    assert(false);
  }

  struct Dump_visitor : Const_syntax_visitor<Dump_visitor, void>
  {
    Dump_visitor(std::ostream& os)
      : os(os), depth(0)
    { }

    // RAII class to help indenting.
    struct Indent
    {
      Indent(Dump_visitor& d)
        : d(d)
      {
        ++d.depth;
      }

      ~Indent()
      {
        --d.depth;
      }

      Dump_visitor& d;
    };

    void visit_syntax(Syntax const* s)
    {
      os << std::string(depth * 2, ' ') << s->class_name() << '\n';
    }

    void visit_sequence(Sequence_syntax const* s)
    {
      visit_syntax(s);
      Indent indent(*this);
      for (Syntax const* ss : s->terms())
        visit(ss);
    }

    void visit_file(File_syntax const* s)
    {
      visit_syntax(s);
      Indent indent(*this);
      visit(s->declarations());
    }

    std::ostream& os;
    int depth;
  };  

  void dump_syntax(Syntax const* s)
  {
    Dump_visitor v(std::cerr);
    v.visit(s);
  }

} // namespace
