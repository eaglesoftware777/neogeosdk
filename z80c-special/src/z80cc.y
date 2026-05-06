%define parse.error detailed
%token ID NUM ASM
%token VOID UNSIGNED CHAR IF ELSE WHILE RETURN
%left '|' '^' '&'
%left EQ NE '<' '>' LE GE
%left '+' '-'
%%
program: items ;
items: /* empty */ | items item ;
item: ASM | type ID ';' | type ID '(' params ')' block ;
type: VOID | UNSIGNED CHAR ;
params: VOID | /* empty */ | param_list ;
param_list: type ID | param_list ',' type ID ;
block: '{' statements '}' ;
statements: /* empty */ | statements statement ;
statement: ASM | block | UNSIGNED CHAR ID ';' | expr '=' expr ';' | expr ';' | RETURN opt_expr ';' | WHILE '(' expr ')' statement | IF '(' expr ')' statement opt_else ;
opt_else: /* empty */ | ELSE statement ;
opt_expr: /* empty */ | expr ;
expr: primary | expr '+' primary | expr '-' primary | expr '&' primary | expr '|' primary | expr '^' primary | expr EQ primary | expr NE primary | expr '<' primary | expr '>' primary | expr LE primary | expr GE primary ;
primary: NUM | ID | ID '(' args ')' | '(' expr ')' ;
args: /* empty */ | arg_list ;
arg_list: expr | arg_list ',' expr ;
%%
