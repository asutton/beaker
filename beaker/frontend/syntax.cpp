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

  // Syntax::children

  namespace
  {
    // NOTE: We can technically deduce R from V, but should we? It doesn't
    // really improve clarity.
    template<Visitor_type V, typename R>
    struct Children_visitor : Syntax_visitor_base<Children_visitor<V, R>, V, R>
    {
      using Base = Syntax_visitor_base<Children_visitor<V, R>, V, R>;

      template<typename T>
      using Ptr = Base::template Ptr<T>;
      
      R visit_atom(Ptr<Atom_syntax> s)
      {
        return {};
      }

      R visit_unary(Ptr<Unary_syntax> s)
      {
        return s->operands();
      }

      R visit_binary(Ptr<Binary_syntax> s)
      {
        return s->operands();
      }

      R visit_ternary(Ptr<Ternary_syntax> s)
      {
        return s->operands();
      }
    
      R visit_multiary(Ptr<Multiary_syntax> s)
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
      Source_range visit_atom(Atom_syntax const* s)
      {
        Source_location start = s->token().start_location();
        Source_location end = s->token().end_location();
        return {start, end};
      }

      // Locations for lists and sequences.
      Source_range visit_multiary(Multiary_syntax const* s)
      {
        Source_location start = s->operands().front()->location().start;
        Source_location end = s->operands().back()->location().end;
        return {start, end};
      }

      // The range of terms like `( ... )`
      Source_range visit_enclosure(Enclosure_syntax const* s)
      {
        Source_location start = s->open().start_location();
        Source_location end = s->close().end_location();
        return {start, end};
      }

      // The range of terms like `@e`
      Source_range visit_prefix(Prefix_syntax const* s)
      {
        Source_location start = s->operation().start_location();
        Source_location end = s->operand()->location().end;
        return {start, end};
      }

      // The range of terms like `e@`
      Source_range visit_postfix(Postfix_syntax const* s)
      {
        Source_location start = s->operand()->location().start;
        Source_location end = s->operation().end_location();
        return {start, end};
      }

      // The range of terms like `e0 @ e1`
      Source_range visit_infix(Infix_syntax const* s)
      {
        Source_location start = s->lhs()->location().start;
        Source_location end = s->rhs()->location().end;
        return {start, end};
      }

      // The range of compound type constructors `ctor e1 e2`
      Source_range visit_constructor(Constructor_syntax const* s)
      {
        Source_location start = s->type().start_location();
        Source_location end = s->specifier()->location().end;
        return {start, end};
      }

      // The range of compound postfix expressions `e1 e2`
      Source_range visit_application(Application_syntax const* s)
      {
        Source_location start = s->applicant()->location().start;
        Source_location end = s->arguments()->location().end;
        return {start, end};
      }

      // The range of declarations depends on the declarative form.
      Source_range visit_declaration(Declaration_syntax const* s)
      {
        // The start of a declartion is either the introducer (for declarations
        // and non-special parameters) or the first valid subtree (parameters
        // may omit the name).
        Source_location start;
        if (Token tok = s->introducer())
          start = tok.start_location();
        else
          start = first_nonnull(s->operands())->location().start;
        
        // The end declaration is that of the last valid subtree (the
        // initializer may be omitted).
        Source_location end = last_nonnull(s->operands())->location().end;

        return {start, end};
      }

      // The range of a file is that of its declaration sequence.
      Source_range visit_file(File_syntax const* s)
      {
        return s->declarations()->location();
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

      void visit_atom(Atom_syntax const* s)
      {
        os << ' ' << s->spelling();
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
        os << std::string(depth * 2, ' ') << s->class_name();
        
        // Print the location of the node.
        os << ' ' << '@' << s->location();
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
      void visit_syntax(Syntax const* s)
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
