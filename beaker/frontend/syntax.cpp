#include <beaker/frontend/syntax.hpp>

#include <iostream>
#include <string>

namespace beaker
{
  /// Returns the kind name.
  char const* Syntax::kind_name() const
  {
    switch (m_kind) {
#define def_syntax(T, B) \
    case T: \
      return #T;
#include <beaker/frontend/syntax.def>
    default:
      break;
    }
    assert(false);
  }

  // Syntax::children

  namespace
  {
    // TODO: It would be nice if I could define this generically instead of
    // using named overloads.
    template<Visitor_type V, typename R>
    struct Children_visitor : Syntax_visitor_base<Children_visitor<V, R>, V, R>
    {
      using Base = Syntax_visitor_base<Children_visitor<V, R>, V, R>;

      template<typename T>
      using Ptr = Base::template Ptr<T>;
      
      R visit_Atom(Ptr<Atom_syntax> s)
      {
        return {};
      }

      R visit_Unary(Ptr<Unary_syntax> s)
      {
        return s->operands();
      }

      R visit_Binary(Ptr<Binary_syntax> s)
      {
        return s->operands();
      }

      R visit_Ternary(Ptr<Ternary_syntax> s)
      {
        return s->operands();
      }

      R visit_Quaternary(Ptr<Quaternary_syntax> s)
      {
        return s->operands();
      }

      R visit_Multiary(Ptr<Multiary_syntax> s)
      {
        return s->operands();
      }
    };
  } // namespace

  Const_syntax_span Syntax::children() const
  {
    Children_visitor<const_visitor, Const_syntax_span> vis;
    return vis.visit(this);
  }

  Syntax_span Syntax::children()
  {
    Children_visitor<mutable_visitor, Syntax_span> vis;
    return vis.visit(this);
  }

  // Syntax::location

  namespace
  {
    // Returns the first non-null tree in `span`.
    Syntax const* first_nonnull(Const_syntax_span span)
    {
      auto iter = std::find_if(span.begin(), span.end(), [](Syntax const* s) {
        return s != nullptr;
      });
      assert(iter != span.end());
      return *iter;
    }

    // Returns the last non-null tree in `span`.
    Syntax const* last_nonnull(Const_syntax_span span)
    {
      auto iter = std::find_if(span.rbegin(), span.rend(), [](Syntax const* s) {
        return s != nullptr;
      });
      assert(iter != span.rend());
      return *iter;
    }

    struct Location_visitor : Const_syntax_visitor<Location_visitor, Source_range>
    {
      Source_range visit_Atom(Atom_syntax const* s)
      {
        Source_location start = s->token().start_location();
        Source_location end = s->token().end_location();
        return {start, end};
      }

      // Locations for lists and sequences.
      Source_range visit_Multiary(Multiary_syntax const* s)
      {
        Source_location start = s->operands().front()->location().start;
        Source_location end = s->operands().back()->location().end;
        return {start, end};
      }

      // The range of terms like `( ... )`
      Source_range visit_Enclosure(Enclosure_syntax const* s)
      {
        Source_location start = s->open().start_location();
        Source_location end = s->close().end_location();
        return {start, end};
      }

      // The range of terms like `@e`
      Source_range visit_Prefix(Prefix_syntax const* s)
      {
        Source_location start = s->operation().start_location();
        Source_location end = s->operand()->location().end;
        return {start, end};
      }

      // The range of terms like `e@`
      Source_range visit_Postfix(Postfix_syntax const* s)
      {
        Source_location start = s->operand()->location().start;
        Source_location end = s->operation().end_location();
        return {start, end};
      }

      // The range of terms like `e0 @ e1`
      Source_range visit_Infix(Infix_syntax const* s)
      {
        Source_location start = s->lhs()->location().start;
        Source_location end = s->rhs()->location().end;
        return {start, end};
      }

      // The range of compound type constructors `ctor e1 e2`.
      Source_range visit_Constructor(Constructor_syntax const* s)
      {
        Source_location start = s->constructor()->location().start;
        Source_location end = s->result()->location().end;
        return {start, end};
      }

      // The range of compound postfix expressions `e1 e2`.
      Source_range visit_Application(Application_syntax const* s)
      {
        Source_location start = s->applicant()->location().start;
        Source_location end = s->arguments()->location().end;
        return {start, end};
      }

      // The range of declarations depends on the declarative form.
      Source_range visit_Declaration(Declaration_syntax const* s)
      {
        Source_location start = first_nonnull(s->operands())->location().start;
        Source_location end = last_nonnull(s->operands())->location().end;
        return {start, end};
      }

      // The range of a file is that of its declaration sequence.
      Source_range visit_File(File_syntax const* s)
      {
        if (Syntax* ds = s->declarations())
          return ds->location();
        return {};
      }
    };
  } // namespace

  Source_range Syntax::location() const
  {
    Location_visitor v;
    return v.visit(this);
  }

  // Syntax::dump

  namespace
  {
    // Print attributes for a declaration.
    struct Dump_attrs_visitor : Const_syntax_visitor<Dump_attrs_visitor, void>
    {
      Dump_attrs_visitor(std::ostream& os)
        : os(os)
      { }

      void visit_Literal(Literal_syntax const* s)
      {
        os << " value=" << '\'' << s->spelling() << '\'';
      }

      void visit_Identifier(Identifier_syntax const* s)
      {
        os << " identifier=" << '\'' << s->spelling() << '\'';
      }

      void visit_Prefix(Prefix_syntax const* s)
      {
        os << " operator=" << '\'' << s->operation().spelling() << '\'';
      }

      void visit_Postfix(Postfix_syntax const* s)
      {
        os << " operator=" << '\'' << s->operation().spelling() << '\'';
      }

      void visit_Infix(Infix_syntax const* s)
      {
        os << " operator=" << '\'' << s->operation().spelling( )<< '\'';
      }

      void visit_Enclosure(Enclosure_syntax const* s)
      {
        os << " kind=" << '\'' << s->open().spelling() << s->close().spelling() << '\'';
      }

      std::ostream& os;
    };

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

      // Print the header for 
      void start_line(Syntax const* s)
      {
        // Print the name of the node.
        os << std::string(depth * 2, ' ') << s->kind_name();
        
        // Print the location of the node.
        if (Source_range rng = s->location())
          os << ' ' << '@' << rng;
      }

      void end_line(Syntax const* s)
      {
        os << '\n';
      }

      // Dump attributes of `s`
      void visit_attributes(Syntax const* s)
      {
        Dump_attrs_visitor attrs(os);
        attrs.visit(s);
      }

      // Recursively visit the children of `s`.
      void visit_children(Syntax const* s)
      {
        Indent indent(*this);
        for (Syntax const* child : s->children())
          if (child)
            visit(child);
      }

      // Prints information about each node.
      void visit_Syntax(Syntax const* s)
      {
        start_line(s);
        visit_attributes(s);
        end_line(s);
        visit_children(s);
      }

      std::ostream& os;
      int depth;
    };
  }

  void Syntax::dump() const
  {
    Dump_visitor v(std::cerr);
    v.visit(this);
  }

} // namespace
