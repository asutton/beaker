
This grammar spells types from left-to-right as unary prefix operators, but
does not require leading or trailing tokens. However, to be unambiguous types
in function and template types must be recognizable as such.

```
def a : [3] int; # an array-of-3-ints
def p : ^int; # a pointer-to-int
def f : (:int) int; # a function-taking-int-returning-int
def x : [:type] type; # a template-taking-type-instantiating-type
```

Parsing the grammar requires infinite lookahead to identify parameter lists.
The current implementation assumes parameter lists start with `(x:` or `(:`, but
the grammar also allows types to be left-distributed:

```
def f(a, b : int);
```

In case like this, we'd have to scan the list looking for parameter
declarations. This is effectively a tentative parse.

Note that multiparameter declarations like this are potentially ambiguous.
We generally assume that un-typed parameters implicitly have type `auto`.
However, explicitly typing a subsequent parameter changes that.

The full grammar of the language follows:

```
file:
    declaration-seq?

declaration-seq:
    declaration
    declaration-seq declaration

declaration:
    definition-declaration


definition-declaration:
    def declarator-list type-clause ;
    def declarator-list : initializer-clause
    def declarator-list type-clause initializer-clause

type-clause;
    : type

initializer-clause:
    = expression ;
    { statement-seq }

parameter:
    identifier : type
    identifier : type = expression
    identifeir : = expression
    : type
    : type = expression

declarator:
    postfix-expression

type-expression:
    prefix-expression

expression:
    infix-expression

infix-expression:
    implication-expression

implication-expression:
    logical-or-expression
    logical-or-expression -> implication-expression

logical-or-expression:
    logical-and-expression
    logical-or-expression or logical-and-expression

logical-and-expression:
    equality-expression
    logical-and-expression and equality-expression

equality-expression:
    relational-expression
    equality-expression == relational-expression
    equality-expression != relational-expression

relational-expression:
    additive-expression
    relational-expression < additive-expression
    relational-expression > additive-expression
    relational-expression <= additive-expression
    relational-expression >= additive-expression

additive-expression:
    multiplicative-expression
    additive-expression < multiplicative-expression
    additive-expression > multiplicative-expression

multiplicative-expression:
    prefix-exprssion
    multiplicative-expression < prefix-exprssion
    multiplicative-expression > prefix-exprssion

prefix-expression:
    postfix-expression
    [ expression-group? ] prefix-expression
    ( parameter-group? ) prefix-expression
    const prefix-exprssion
    ^ prefix-expression
    - prefix-expression
    + prefix-expression
    not prefix-expression

postfix-expression:
    primary-expression
    postfix-expression ( expression-list? )
    postfix-expression [ expression-list? ]
    postfix-expression .
    postfix-expression ^

primary-expression:
    literal
    ( expression-list )
    id-expression

literal:
    integer
    int
    bool
    type

id-expression:
    identifier

expression-group:
    expression-list
    expression-group ; expression-list

expression-list:
    parameter-expression
    expression-list , parameter-expression

parameter-or-expression:
    parameter
    expression

parameter-group:
    parameter-list
    parameter-group ; parameter-list

parameter-list:
    parameter
    parameter-list , parameter


statement-seq
    statement
    statement-seq statement

statement:
    declaration-statement
    expression-statement
``` 
