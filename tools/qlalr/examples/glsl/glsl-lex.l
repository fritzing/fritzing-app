
%{
#include <cassert>
#define YY_DECL int GLSLParser::nextToken()
%}

%option noyywrap

hex [0-9a-fA-F]
dec [1-9][0-9]*
oct [0-7]
digit [0-9]

fract {digit}*\.{digit}+|{digit}+\.
exp [eE][+-]?{digit}+

hexfract {hex}*\.{hex}+|{hex}+\.
binexp [pP][+-]?{digit}+

icst ({dec}|0{oct}*|0[xX]{hex}+)

%%

[\n]					{ ++context.line; }
[ \t\r]+				{ /* skip */ }

"+=" { return ADD_ASSIGN; }
"&" { return AMPERSAND; }
"&=" { return AND_ASSIGN; }
"&&" { return AND_OP; }
"attribute" { return ATTRIBUTE; }
"!" { return BANG; }
"bool" { return BOOL; }
"true" { return BOOLCONSTANT; }
"false" { return BOOLCONSTANT; }
"break" { return BREAK; }
"bvec2" { return BVEC2; }
"bvec3" { return BVEC3; }
"bvec4" { return BVEC4; }
":" { return COLON; }
"," { return COMMA; }
"const" { return CONST; }
"continue" { return CONTINUE; }
"-" { return DASH; }
"--" { return DEC_OP; }
"discard" { return DISCARD; }
"/=" { return DIV_ASSIGN; }
"do" { return DO; }
"." { return DOT; }
"else" { return ELSE; }
"=" { return EQUAL; }
"==" { return EQ_OP; }
"float" { return FLOAT; }
"for" { return FOR; }
">=" { return GE_OP; }
"if" { return IF; }
"in" { return IN; }
"++" { return INC_OP; }
"inout" { return INOUT; }
"int" { return INT; }
"ivec2" { return IVEC2; }
"ivec3" { return IVEC3; }
"ivec4" { return IVEC4; }
"<" { return LEFT_ANGLE; }
"<<=" { return LEFT_ASSIGN; }
"{" { return LEFT_BRACE; }
"[" { return LEFT_BRACKET; }
"<<" { return LEFT_OP; }
"(" { return LEFT_PAREN; }
"<=" { return LE_OP; }
"mat2" { return MAT2; }
"mat3" { return MAT3; }
"mat4" { return MAT4; }
"%=" { return MOD_ASSIGN; }
"*=" { return MUL_ASSIGN; }
"!=" { return NE_OP; }
"|=" { return OR_ASSIGN; }
"||" { return OR_OP; }
"out" { return OUT; }
"%" { return PERCENT; }
"+" { return PLUS; }
"?" { return QUESTION; }
"return" { return RETURN; }
">" { return RIGHT_ANGLE; }
">>=" { return RIGHT_ASSIGN; }
"}" { return RIGHT_BRACE; }
"]" { return RIGHT_BRACKET; }
">>" { return RIGHT_OP; }
")" { return RIGHT_PAREN; }
"sampler1D" { return SAMPLER1D; }
"sampler1DShadow" { return SAMPLER1DSHADOW; }
"sampler2D" { return SAMPLER2D; }
"sampler2DShadow" { return SAMPLER2DSHADOW; }
"sampler3D" { return SAMPLER3D; }
"samplerCube" { return SAMPLERCUBE; }
";" { return SEMICOLON; }
"/" { return SLASH; }
"*" { return STAR; }
"struct" { return STRUCT; }
"-=" { return SUB_ASSIGN; }
"~" { return TILDE; }
"uniform" { return UNIFORM; }
"varying" { return VARYING; }
"vec2" { return VEC2; }
"vec3" { return VEC3; }
"vec4" { return VEC4; }
"|" { return VERTICAL_BAR; }
"void" { return VOID; }
"while" { return WHILE; }
"^=" { return XOR_ASSIGN; }
"^" { return XOR_OP; }

#[ \t]+[0-9]+.* {
  char *eptr = 0;
  context.line = (int) strtod(&yytext[1], &eptr);
  QString fn = QString::fromUtf8(eptr).trimmed();
  if (fn.length() > 2)
    context.fileName = fn.mid(1, fn.length()-2);
}

#.* {
  /* skip */
}

[_a-zA-Z][_a-zA-Z0-9]* {
  yylval.s = intern (yytext);

  if (isTypename (yylval.s))
    return TYPE_NAME;

  return IDENTIFIER;
}

{icst} {
  yylval.i = (int) strtol (yytext, 0, 0);
  return INTCONSTANT;
}

{icst}[uU] {
  yylval.u = (unsigned) strtoul (yytext, 0, 0);
  return INTCONSTANT;
}

{icst}[uU][lL] {
  yylval.ul = strtoul (yytext, 0, 0);
  return INTCONSTANT;
}

{icst}[lL][uU] {
  yylval.ul = strtoul (yytext, 0, 0);
  return INTCONSTANT;
}

{icst}[lL] {
  yylval.l = strtol (yytext, 0, 0);
  return INTCONSTANT;
}

{icst}[uU](ll|LL) {
  yylval.l = strtoull (yytext, 0, 0);
  return INTCONSTANT;
}

{icst}(ll|LL) {
  yylval.l = strtoll (yytext, 0, 0);
  return INTCONSTANT;
}

{icst}(ll|LL)[uU] {
  yylval.l = strtoull (yytext, 0, 0);
  return INTCONSTANT;
}

{fract}{exp}?[flFL]? {
  yylval.f = strtof (yytext, 0);
  return FLOATCONSTANT;
}

{digit}+{exp}[flFL]? {
  yylval.f = strtof (yytext, 0);
  return FLOATCONSTANT;
}

0[xX]{hexfract}{binexp}[flFL]? {
  yylval.f = strtof (yytext, 0);
  return FLOATCONSTANT;
}

0[xX]{hex}+{binexp}[flFL]? {
  yylval.f = strtof (yytext, 0);
  return FLOATCONSTANT;
}

. {
  fprintf (stderr, "invalid char: %d\n", yytext [0]);
  return ERROR;
}


%%

