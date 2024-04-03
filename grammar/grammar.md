```
compound_statement: '{' '}'          // empty, i.e. no statement
      |      '{' statement '}'
      |      '{' statement statements '}'
      ;

 statement: print_statement
      |     declaration
      |     assignment_statement
      |     if_statement
      ;

 print_statement: 'print' expression ';'  ;
 declaration: 'int' identifier ';'  ;
 assignment_statement: identifier '=' expression ';'   ;

 if_statement: if_head
      |        if_head 'else' compound_statement
      ;

 if_head: 'if' '(' true_false_expression ')' compound_statement  ;

 identifier: T_IDENT ;

 expression: additive_expression
    ;

 additive_expression:
           multiplicative_expression
    |      additive_expression '+' multiplicative_expression
    |      additive_expression '-' multiplicative_expression
    ;

 multiplicative_expression:
           number
    |      number '*' multiplicative_expression
    |      number '/' multiplicative_expression
    ;

 number: T_INTLIT
    ;
```
