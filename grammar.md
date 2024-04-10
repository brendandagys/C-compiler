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
 
 global_declarations : global_declarations 
      | global_declaration global_declarations
      ;

 global_declaration: function_declaration | variable_declaration   ;

 function_declaration: type identifier '(' ')' compound_statement   ;

 variable_declaration: type identifier_list ';'   ;

 type: type_keyword opt_pointer   ;

 type_keyword: 'void' | 'char' | 'int' | 'long'   ;

 opt_pointer: <empty> | '*' opt_pointer   ;

 identifier_list: identifier | identifier ',' identifier_list   ;

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
