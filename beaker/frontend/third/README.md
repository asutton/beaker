
The file extension for this language is `.bkr3`. The compiler can also be run
with `-language third`. The grammar is:

This langauge requires neither an explicit prefix for unary type consructors,
nor a suffix operand. However, the grammar for function type constructors
requires explicit annotations for types (e.g., `(:int)`). We don't need to
do this for array and template type constructors because there is no
primary expression that starts with a `[`.

```
file:
    declaration-seq?

declaration-seq:
    declaration
    declaration-seq declaration

declaration:
    definition-declaration

definition-declaration:
    def declarator : type ;
    def declarator : = expression ;
    def declarator : type = expression ;

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
``` 
