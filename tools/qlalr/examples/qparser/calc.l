
%option noyywrap

%{
#include "calc_parser.h"
#include <cstdlib>

#define YY_DECL int CalcParser::nextToken()
%}

%%

[ \t\n]     { /* eat me */ }
[0-9]+      { sym(1) = atoi (yytext); return Token_number; }
"("         { return Token_lparen; }
")"         { return Token_rparen; }
"+"         { return Token_plus; }
"-"         { return Token_minus; }

%%
