```
 compound_statement: '{' '}'          // empty, i.e. no statement
      |      '{' statement '}'
      |      '{' statement statements '}'
      ;

 statement: declaration
      |     function_call
      |     if_statement
      |     while_statement
      |     for_statement
      |     return_statement
      ;

 global_declarations : global_declarations 
      | global_declaration global_declarations
      ;

 global_declaration: function_declaration | variable_declaration   ;

 function_declaration: type identifier '(' parameter_list ')' ';'
      | type identifier '(' parameter_list ')' compound_statement
      ;

 expression_list: <null>
      | expression
      | expression ',' expression_list
      ;

 param_declaration: <null>
      | variable_declaration
      | variable_declaration ',' param_declaration

 variable_declaration: type identifier_list ';'
      | type identifier '[' P_INTLIT ']' ';'
      ;

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

 primary_expression
    : T_IDENT
    | T_INTLIT
    | T_STRLIT
    | '(' expression ')'
    ;

 postfix_expression
    : primary_expression
    | postfix_expression '[' expression ']'
    | postfix_expression '(' expression ')'
    | postfix_expression '++'
    | postfix_expression '--'
    ;

 prefix_expression
    : postfix_expression
    | '++' prefix_expression
    | '--' prefix_expression
    | prefix_operator prefix_expression
    ;
  
 prefix_operator: '&' | '*' | '-' | '~' | '!'   ;

 expression
    : additive_expression
    | primary_expression
    | prefix_expression
    ;

 additive_expression
    : multiplicative_expression
    | additive_expression '+' multiplicative_expression
    | additive_expression '-' multiplicative_expression
    ;

 multiplicative_expression
    : prefix_expression
    | multiplicative_expression '*' prefix_expression
    | multiplicative_expression '/' prefix_expression
    | multiplicative_expression '%' prefix_expression
    ;

 identifier: T_IDENT ;

 CONSTANT: number
    ;

 number: T_INTLIT
    ;
```
