```
 compound_statement: '{' '}'          // empty, i.e. no statement
      |      '{' statement '}'
      |      '{' statement statements '}'
      ;

 statement: print_statement
      |     declaration
      |     assignment_statement
      |     function_call
      |     if_statement
      |     while_statement
      |     for_statement
      |     return_statement
      ;

 print_statement: 'print' expression ';'  ;
 
 declaration: 'int' identifier ';'  ;
 
 function_declaration: 'void' identifier '(' ')' compound_statement  ;
 
 assignment_statement: identifier '=' expression ';'   ;

 if_statement: if_head
      |        if_head 'else' compound_statement
      ;
    
 while_statement: 'while' '(' true_false_expression ')' compound_statement ;

 for_statement: 'for' '(' preop_statement ';' true_false_expression ';' postop_statement ')' compound_statement  ;

 preop_statement:  statement  ;
 postop_statement: statement  ;

 if_head: 'if' '(' true_false_expression ')' compound_statement  ;

 function_call: identifier '(' expression ')'   ;

 return_statement: 'return' '(' expression ')'  ;

 identifier: T_IDENT ;

 prefix_expression: primary
    | '*' prefix_expression
    | '&' prefix_expression
    ;

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
