
The file extension for this language is `.bkr2`. The compiler can also be run
with `-language second`. 

This grammar allows parameter groups for array types, which is kind of
unfortunate, but we can filter on it. In fact, the grammar readily conflates
template and array types:

```
[t:type] => type # OK: A template type
[t:type] type # Syntacially fine, semantically ill-formed
```

The grammar is:

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
    logical-or-expression

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
    prefix-expression
    multiplicative-expression < prefix-exprssion
    multiplicative-expression > prefix-exprssion

prefix-expression:
    postfix-expression
    [ expression-group ] prefix-expression
    [ expression-group ] => prefix-expression
    ( expression-group ) -> prefix-expression
    const prefix-exprssion
    ^ prefix-expression
    - prefix-expression
    + prefix-expression
    not prefix-expression

postfix-expression:
    primary-expression
    postfix-expression ( expression-list )
    postfix-expression [ expression-list ]
    postfix-expression .
    postfix-expression ^

primary-expression:
    literal
    id-expression
    ( expression-list )

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
``` 
