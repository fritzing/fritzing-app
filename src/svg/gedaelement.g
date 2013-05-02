%parser         GedaElementGrammar
%decl           gedaelementparser.h
%impl           gedaelementparser.cpp

%token ELEMENT
%token PAD
%token PIN
%token MARK
%token ELEMENTLINE
%token ELEMENTARC
%token ATTRIBUTE
%token LEFTPAREN
%token RIGHTPAREN
%token LEFTBRACKET
%token RIGHTBRACKET
%token NUMBER
%token STRING
%token HEXNUMBER

%start geda_element

/:
/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-08 Fachhochschule Potsdam - http://fh-potsdam.de

Fritzing is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Fritzing is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************

$Revision: 1671 $:
$Author: cohen@irascible.com $:
$Date: 2008-11-28 12:20:31 +0100 (Fri, 28 Nov 2008) $

********************************************************************/

#ifndef GEDAELEMENTPARSER_H
#define GEDAELEMENTPARSER_H

#include <QVariant>
#include <QVector>
#include "gedaelementgrammar_p.h"

class GedaElementLexer;

class GedaElementParser: public $table
{
public:
    GedaElementParser();
    ~GedaElementParser();

    bool parse(GedaElementLexer *lexer);
    QVector<QVariant> & symStack();
    QString errorMessage() const;
    QVariant result() const;

private:
    void reallocateStack();
    int m_tos;
    QVector<int> m_stateStack;
    QVector<QVariant> m_symStack;
    QString m_errorMessage;
    QVariant m_result;
};

#endif // GEDAELEMENTPARSER_H
:/

/.
/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-08 Fachhochschule Potsdam - http://fh-potsdam.de

Fritzing is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Fritzing is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************

$Revision: 1671 $:
$Author: cohen@irascible.com $:
$Date: 2008-11-28 12:20:31 +0100 (Fri, 28 Nov 2008) $

********************************************************************/

#include <QDebug>
#include "gedaelementparser.h"
#include "gedaelementlexer.h"

GedaElementParser::GedaElementParser()
{
}

GedaElementParser::~GedaElementParser()
{
}

QVector<QVariant> & GedaElementParser::symStack() {
	return m_symStack;
}

void GedaElementParser::reallocateStack()
{
    int size = m_stateStack.size();
    if (size == 0)
        size = 128;
    else
        size <<= 1;

    m_stateStack.resize(size);
}

QString GedaElementParser::errorMessage() const
{
    return m_errorMessage;
}

QVariant GedaElementParser::result() const
{
    return m_result;
}

bool GedaElementParser::parse(GedaElementLexer *lexer)
{
  const int INITIAL_STATE = 0;

  int yytoken = -1;

  reallocateStack();

  m_tos = 0;
  m_stateStack[++m_tos] = INITIAL_STATE;

  while (true) {
      const int state = m_stateStack.at(m_tos);
      if (yytoken == -1 && - TERMINAL_COUNT != action_index [state])
        yytoken = lexer->lex();
      int act = t_action (state, yytoken);
      if (act == ACCEPT_STATE)
        return true;

      else if (act > 0) {
          if (++m_tos == m_stateStack.size())
            reallocateStack();
          m_stateStack[m_tos] = act;
          yytoken = -1;
      } else if (act < 0) {
          int r = - act - 1;

          m_tos -= rhs [r];
          act = m_stateStack.at(m_tos++);

          switch (r) {
./


geda_element ::=  element_command element_command_sequence sub_element_sequence ;
/. case $rule_number: {
    qDebug() << "got geda_element ";
} break; ./

sub_element_sequence ::= sub_element_sequence_paren | sub_element_sequence_bracket ;
/. case $rule_number: {
} break; ./

sub_element_sequence_paren ::= LEFTPAREN sub_element_groups RIGHTPAREN ;
/. case $rule_number: {
	m_symStack.append(QChar(')'));
} break; ./

sub_element_sequence_bracket ::= LEFTBRACKET sub_element_groups RIGHTBRACKET ;
/. case $rule_number: {
	m_symStack.append(QChar(']'));
} break; ./

element_command_sequence ::= element_command_sequence_paren | element_command_sequence_bracket ;
/. case $rule_number: {
    qDebug() << "    got element_command sequence ";
} break; ./

element_command_sequence_paren ::= LEFTPAREN element_arguments RIGHTPAREN ;
/. case $rule_number: {
	m_symStack.append(QChar(')'));
    qDebug() << "    got element_command sequence ";
} break; ./

element_command_sequence_bracket ::= LEFTBRACKET element_arguments RIGHTBRACKET ;
/. case $rule_number: {
	m_symStack.append(QChar(']'));
    qDebug() << "    got element_command sequence ";
} break; ./

element_arguments ::= SFlags description pcb_name value mark_x mark_y text_x text_y text_direction text_scale SFlags ;
/. case $rule_number: {
    qDebug() << "    got element_arguments ";
} break; ./

sub_element_groups ::= sub_element_group | sub_element_group sub_element_groups ;
/. case $rule_number: {
    qDebug() << "    got sub_element_groups ";
} break; ./

sub_element_group ::= pin_element | pad_element | element_arc_element | element_line_element | mark_element | attribute_element;
/. case $rule_number: {
    qDebug() << "    got sub_element_group ";
} break; ./

mark_element ::= mark_command mark_sequence ;
/. case $rule_number: {
    qDebug() << "got mark_element ";
} break; ./

mark_sequence ::= mark_paren_sequence | mark_bracket_sequence ;
/. case $rule_number: {
    qDebug() << "    got mark_sequence ";
} break; ./

mark_paren_sequence ::= LEFTPAREN mark_arguments RIGHTPAREN ;
/. case $rule_number: {
	m_symStack.append(QChar(')'));
    qDebug() << "    got mark_paren_sequence ";
} break; ./

mark_bracket_sequence ::= LEFTBRACKET mark_arguments RIGHTBRACKET ;
/. case $rule_number: {
	m_symStack.append(QChar(']'));
    qDebug() << "    got mark_bracket_sequence ";
} break; ./

mark_arguments ::= x y ;
/. case $rule_number: {
    qDebug() << "    got mark_arguments";
} break; ./

pin_element ::= pin_command pin_sequence ;
/. case $rule_number: {
    qDebug() << "got pin_element ";
} break; ./

pin_sequence ::= pin_paren_sequence | pin_bracket_sequence ;
/. case $rule_number: {
    qDebug() << "    got pin_sequence ";
} break; ./

pin_paren_sequence ::= LEFTPAREN pin_arguments RIGHTPAREN ;
/. case $rule_number: {
	m_symStack.append(QChar(')'));
    qDebug() << "    got pin_paren_sequence ";
} break; ./

pin_bracket_sequence ::= LEFTBRACKET pin_arguments RIGHTBRACKET ;
/. case $rule_number: {
	m_symStack.append(QChar(']'));
    qDebug() << "    got pin_bracket_sequence ";
} break; ./

pin_arguments ::= pin_arguments_1 | pin_arguments_2 | pin_arguments_3 | pin_arguments_4;
/. case $rule_number: {
    qDebug() << "    got pin_arguments ";
} break; ./

pin_arguments_1 ::= x y Thickness Clearance Mask DrillHole Name pin_number SFlags ;
/. case $rule_number: {
    qDebug() << "    got pin_arguments 1";
} break; ./

pin_arguments_2 ::= x y Thickness DrillHole Name pin_number NFlags ;
/. case $rule_number: {
    qDebug() << "    got pin_arguments 2";
} break; ./

pin_arguments_3 ::= x y Thickness  DrillHole Name NFlags ;
/. case $rule_number: {
    qDebug() << "    got pin_arguments 3";
} break; ./

pin_arguments_4 ::= x y Thickness Name NFlags ;
/. case $rule_number: {
    qDebug() << "    got pin_arguments 4 ";
} break; ./

pad_element ::= pad_command pad_sequence ;
/. case $rule_number: {
    qDebug() << "got pad_element ";
} break; ./

pad_sequence ::= pad_paren_sequence | pad_bracket_sequence ;
/. case $rule_number: {
    qDebug() << "    got pad_sequence ";
} break; ./

pad_paren_sequence ::= LEFTPAREN pad_arguments RIGHTPAREN ;
/. case $rule_number: {
	m_symStack.append(QChar(')'));
    qDebug() << "    got pad_paren_sequence ";
} break; ./

pad_bracket_sequence ::= LEFTBRACKET pad_arguments RIGHTBRACKET ;
/. case $rule_number: {
	m_symStack.append(QChar(']'));
    qDebug() << "    got pad_bracket_sequence ";
} break; ./

pad_arguments ::= pad_arguments_1 | pad_arguments_2 | pad_arguments_3 ;
/. case $rule_number: {
    qDebug() << "    got pad_arguments ";
} break; ./

pad_arguments_1 ::= x1 y1 x2 y2 Thickness Clearance Mask Name pad_number SFlags ;
/. case $rule_number: {
    qDebug() << "    got pad_arguments 1";
} break; ./

pad_arguments_2 ::= x1 y1 x2 y2 Thickness Name pad_number NFlags ;
/. case $rule_number: {
    qDebug() << "    got pad_arguments 2";
} break; ./

pad_arguments_3 ::= x1 y1 x2 y2 Thickness Name NFlags ;
/. case $rule_number: {
    qDebug() << "    got pad_arguments 3";
} break; ./

element_line_element ::= element_line_command element_line_sequence ;
/. case $rule_number: {
    qDebug() << "got element_line_element ";
} break; ./

element_line_sequence ::= element_line_paren_sequence | element_line_bracket_sequence ;
/. case $rule_number: {
    qDebug() << "    got element_line_sequence ";
} break; ./

element_line_paren_sequence ::= LEFTPAREN element_line_arguments RIGHTPAREN ;
/. case $rule_number: {
	m_symStack.append(QChar(')'));
    qDebug() << "    got element_line_paren_sequence ";
} break; ./

element_line_bracket_sequence ::= LEFTBRACKET element_line_arguments RIGHTBRACKET ;
/. case $rule_number: {
	m_symStack.append(QChar(']'));
    qDebug() << "    got element_line_bracket_sequence ";
} break; ./

element_line_arguments ::= x1 y1 x2 y2 Thickness ;
/. case $rule_number: {
    qDebug() << "    got element_line_arguments ";
} break; ./

element_arc_element ::= element_arc_command element_arc_sequence ;
/. case $rule_number: {
    qDebug() << "got element_arc_element ";
} break; ./

element_arc_sequence ::= element_arc_paren_sequence | element_arc_bracket_sequence ;
/. case $rule_number: {
    qDebug() << "    got element_arc_sequence ";
} break; ./

element_arc_paren_sequence ::= LEFTPAREN element_arc_arguments RIGHTPAREN ;
/. case $rule_number: {
	m_symStack.append(QChar(')'));
    qDebug() << "    got element_arc_paren_sequence ";
} break; ./

element_arc_bracket_sequence ::= LEFTBRACKET element_arc_arguments RIGHTBRACKET ;
/. case $rule_number: {
	m_symStack.append(QChar(']'));
    qDebug() << "    got element_arc_bracket_sequence ";
} break; ./

element_arc_arguments ::= x y Width Height StartAngle Delta Thickness ;
/. case $rule_number: {
    qDebug() << "    got element_arc_arguments ";
} break; ./

attribute_element ::= attribute_command attribute_sequence ;
/. case $rule_number: {
    qDebug() << "got attribute_element ";
} break; ./

attribute_sequence ::= attribute_paren_sequence ;
/. case $rule_number: {
    qDebug() << "    got attribute_sequence ";
} break; ./

attribute_paren_sequence ::= LEFTPAREN attribute_arguments RIGHTPAREN ;
/. case $rule_number: {
	m_symStack.append(QChar(')'));
    qDebug() << "    got attribute_paren_sequence ";
} break; ./

attribute_arguments ::= Name value ;
/. case $rule_number: {
    qDebug() << "    got attribute_arguments ";
} break; ./

pad_number ::= string_value ;
/. case $rule_number: {
} break; ./

x ::= number_value ;
/. case $rule_number: {
} break; ./

x1 ::= number_value ;
/. case $rule_number: {
} break; ./

x2 ::= number_value ;
/. case $rule_number: {
} break; ./

y ::= number_value ;
/. case $rule_number: {
} break; ./

y1 ::= number_value ;
/. case $rule_number: {
} break; ./

y2 ::= number_value ;
/. case $rule_number: {
} break; ./

Thickness ::= number_value ;
/. case $rule_number: {
} break; ./

Clearance ::= number_value ;
/. case $rule_number: {
} break; ./

Mask ::= number_value ;
/. case $rule_number: {
} break; ./

DrillHole ::= number_value ;
/. case $rule_number: {
} break; ./

Name ::= string_value ;
/. case $rule_number: {
} break; ./

pin_number ::= string_value ;
/. case $rule_number: {
} break; ./

Width ::= number_value ;
/. case $rule_number: {
} break; ./

Height ::= number_value ;
/. case $rule_number: {
} break; ./

StartAngle ::= number_value ;
/. case $rule_number: {
} break; ./

Delta ::= number_value ;
/. case $rule_number: {
} break; ./

SFlags ::= string_value | hex_number_value ;
/. case $rule_number: {
} break; ./

NFlags ::= hex_number_value ;
/. case $rule_number: {
} break; ./

description ::= string_value ;
/. case $rule_number: {
} break; ./

pcb_name ::= string_value ;
/. case $rule_number: {
} break; ./

value ::= string_value ;
/. case $rule_number: {
} break; ./

mark_x ::= number_value ;
/. case $rule_number: {
} break; ./

mark_y ::= number_value ;
/. case $rule_number: {
} break; ./

text_x ::= number_value ;
/. case $rule_number: {
} break; ./

text_y ::= number_value ;
/. case $rule_number: {
} break; ./

text_direction ::= number_value ;
/. case $rule_number: {
} break; ./

text_scale ::= number_value ;
/. case $rule_number: {
} break; ./

number_value ::= NUMBER ;
/. 
case $rule_number: {
    qDebug() << "        got NUMBER " << lexer->currentNumber();
    m_symStack.append(lexer->currentNumber());
} break; 
./

hex_number_value ::= HEXNUMBER ;
/. 
case $rule_number: {
    qDebug() << "        got HEXNUMBER " << lexer->currentNumber();
    m_symStack.append(lexer->currentNumber());
} break; 
./

string_value ::= STRING ;
/. 
case $rule_number: {
    qDebug() << "        got STRING " << lexer->currentString();
    m_symStack.append(lexer->currentString());
} break; 
./

element_command ::= ELEMENT ;
/. 
case $rule_number: {
    qDebug() << "got ELEMENT command ";
    m_symStack.append(lexer->currentCommand());
} break; 
./

pin_command ::= PIN ;
/. 
case $rule_number: {
    qDebug() << "got PIN command ";
    m_symStack.append(lexer->currentCommand());
} break; 
./

pad_command ::= PAD ;
/. 
case $rule_number: {
    qDebug() << "got PAD command ";
    m_symStack.append(lexer->currentCommand());
} break; 
./

mark_command::= MARK ;
/. 
case $rule_number: {
    qDebug() << "got MARK command ";
    m_symStack.append(lexer->currentCommand());
} break; 
./

element_line_command ::= ELEMENTLINE ;
/. 
case $rule_number: {
    qDebug() << "got ELEMENTLINE command ";
    m_symStack.append(lexer->currentCommand());
} break; 
./

element_arc_command ::= ELEMENTARC ;
/. 
case $rule_number: {
    qDebug() << "got ELEMENTARC command ";
    m_symStack.append(lexer->currentCommand());
} break; 
./

attribute_command ::= ATTRIBUTE ;
/. 
case $rule_number: {
    qDebug() << "got ATTRIBUTE command ";
    m_symStack.append(lexer->currentCommand());
} break; 
./


/.
          } // switch

          m_stateStack[m_tos] = nt_action(act, lhs[r] - TERMINAL_COUNT);

      } else {
          int ers = state;
          int shifts = 0;
          int reduces = 0;
          int expected_tokens[3];
          for (int tk = 0; tk < TERMINAL_COUNT; ++tk) {
              int k = t_action(ers, tk);

              if (! k)
                continue;
              else if (k < 0)
                ++reduces;
              else if (spell[tk]) {
                  if (shifts < 3)
                    expected_tokens[shifts] = tk;
                  ++shifts;
              }
          }

          m_errorMessage.clear();
          if (shifts && shifts < 3) {
              bool first = true;

              for (int s = 0; s < shifts; ++s) {
                  if (first)
                    m_errorMessage += QLatin1String("Expected ");
                  else
                    m_errorMessage += QLatin1String(", ");

                  first = false;
                  m_errorMessage += QLatin1String("`");
                  m_errorMessage += QLatin1String(spell[expected_tokens[s]]);
                  m_errorMessage += QLatin1String("'");
              }
          }

          return false;
        }
    }

    return false;
}
./



























