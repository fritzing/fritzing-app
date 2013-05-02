
%parser calc_grammar
%decl calc_parser.h
%impl calc_parser.cpp

%token_prefix Token_
%token number
%token lparen
%token rparen
%token plus
%token minus

%start Goal

/:
#ifndef CALC_PARSER_H
#define CALC_PARSER_H

#include "qparser.h"
#include "calc_grammar_p.h"

class CalcParser: public QParser<CalcParser, $table>
{
public:
  int nextToken();
  void consumeRule(int ruleno);
};

#endif // CALC_PARSER_H
:/





/.
#include "calc_parser.h"

#include <QtDebug>
#include <cstdlib>

void CalcParser::consumeRule(int ruleno)
  {
    switch (ruleno) {
./

Goal: Expression ;
/.
case $rule_number:
  qDebug() << "value:" << sym(1);
  break;
./

PrimaryExpression: number ;
PrimaryExpression: lparen Expression rparen ;
/.
case $rule_number:
  sym(1) = sym (2);
  break;
./

Expression: PrimaryExpression ;

Expression: Expression plus PrimaryExpression;
/.
case $rule_number:
  sym(1) += sym (3);
  break;
./

Expression: Expression minus PrimaryExpression;
/.
case $rule_number:
  sym(1) -= sym (3);
  break;
./



/.
    } // switch
}

#include <cstdio>

int main()
{
  CalcParser p;

  if (p.parse())
    printf("ok\n");
}
./
