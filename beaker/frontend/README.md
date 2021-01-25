
There are a number of different experimental grammars here. The purpose is
to play with the ergonomics of various syntaxes, especially for the writing
of declarations and types.

All of these grammars assume that whitespace is insignificant. All parsers
share the same lexical properties and keywords (for now). Almost all of these
grammars implement Pascal-like declarations `(x : t)`.

There is one constraint on the grammar. It must be able to parse types and
expressions without performing lookup, and ideally without an extra keyword
to denote context.

The parsers are as follows:

- **first**. Declarations are based on a declarator and type specifier. The
  declarator implies how a name can be used. Types are read left-to-right,
  and keywords introduce compound type constructors.
  
  ```
  f(x:int) : int;
  f : func(x:int) : int;
  ```

  Pointers and dereferencing are like Pascal. A pointer type is introduced
  with a prefix `^` (e.g., `^int`) and deferencing is a postfix operator
  (e.g., `p^`). 

- **second**. The same as first, except that (some) compound type constructors
  are followed by a token rather than being introduced by a token.
  
  ```
  f(x:int) : int;
  f : (int) -> int;
  ```

  The type notion is more mathematically common, but requires infinite lookahead
  to disambiguate prefix operators.

- **third**. Like first and second, but tries to eliminate all punctuation
  before and after prefix operators. To make this unambiguous, mapping types
  must use an explicit "type" annotation as part of their type list.

  ```
  f(x:int) : int;
  f : (:int) int; # (:int) is a type constructor, but (int) is a
                  # primary expression
  ```

- **third**. Quite a bit different. This language abandons trying to find
  left-to-right notation for non-mapping types. Mapping types are defined by
  implication operators, and all other types are postfix expressions. Pointers
  and arrays are written as `ptr[t]` and `array[t]` respectively. In the array
  case, the bounds are applied after the type constructor. 

  ```
  f(x:int) : int;
  f : (int) -> int;
  p : ptr[int];
  a : array[int][3][4];
  ```

  Note that this language reverts dereferencing to its conventional prefix
  position, like C/C++.

General observations follow:

Any token used as postfix operator cannot also be an infix operator. This means
that `first` cannot use `^` as the binary XOR operator, which has become common
in many systems programming languages.

For a strict left-to-right grammar for types to be unambigious, it must be
possible to syntactically identify the prefix operator part of the term. This is
trivial if keywords (e.g., `array`, `func`) are used as the prefix operator.
Using a suffix like (e.g., `(...) ->` works, but requires infinite lookeahead.
We can also force parameter lists to look different (e.g., require a `:` to
designate types). That requires some few tokens of lookahead.
Omitting both leads to ambiguities if a primary expression starts with the same
token as the prefix operator. For example:

```
(a)(b)
```

This can be parsed as a prefix oeprator (`(a)`) on a primary expression (`b`).
In that case, we would evaluate `b` first and then apply it to `a`. Or, we can
parse this as a primary expression (`(a)` applied to an argument (`b`), in which
case we evaluate `a` first, and then apply that to `b`. Basically, the grammar
allows two interpretations where the order of terms is inverted:

```
(a)(b)(c)(d)
```

Can be the tree `a -> b -> c -> d` (postfix interpretation) or 
`d -> c -> b -> a` (prefix interpretation).
