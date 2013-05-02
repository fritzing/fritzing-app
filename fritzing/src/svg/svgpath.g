%parser         SVGPathGrammar
%decl           svgpathparser.h
%impl           svgpathparser.cpp

%token EM
%token ZEE
%token EKS
%token EL
%token AITCH
%token VEE
%token CEE
%token ESS
%token KYU
%token TEE
%token AE
%token COMMA
%token NUMBER
%token WHITESPACE

%start path_data

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

$Revision: 3454 $:
$Author: cohen@irascible.com $:
$Date: 2009-09-11 19:39:43 +0200 (Fr, 11. Sep 2009) $

********************************************************************/

#ifndef SVGPATHPARSER_H
#define SVGPATHPARSER_H

#include <QVariant>
#include <QVector>
#include "svgpathgrammar_p.h"

class SVGPathLexer;

class SVGPathParser: public $table
{
public:
    SVGPathParser();
    ~SVGPathParser();

    bool parse(SVGPathLexer *lexer);
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

#endif // SVGPATHPARSER_H
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

$Revision: 3454 $:
$Author: cohen@irascible.com $:
$Date: 2009-09-11 19:39:43 +0200 (Fr, 11. Sep 2009) $

********************************************************************/

#include <QDebug>
#include "svgpathparser.h"
#include "svgpathlexer.h"

SVGPathParser::SVGPathParser()
{
}

SVGPathParser::~SVGPathParser()
{
}

QVector<QVariant> & SVGPathParser::symStack() {
	return m_symStack;
}

void SVGPathParser::reallocateStack()
{
    int size = m_stateStack.size();
    if (size == 0)
        size = 128;
    else
        size <<= 1;

    m_stateStack.resize(size);
}

QString SVGPathParser::errorMessage() const
{
    return m_errorMessage;
}

QVariant SVGPathParser::result() const
{
    return m_result;
}

bool SVGPathParser::parse(SVGPathLexer *lexer)
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


path_data ::=  moveto_drawto_command_groups ;
/. case $rule_number: {
    qDebug() << " got path_data ";
} break; ./

moveto_drawto_command_groups ::= moveto_drawto_command_group | moveto_drawto_command_group moveto_drawto_command_groups ;
/. case $rule_number: {
    //qDebug() << " got moveto_drawto_command_groups ";
} break; ./

moveto_drawto_command_group ::= moveto drawto_commands | moveto;
/. case $rule_number: {
    //qDebug() << " got moveto_drawto_command_group ";
} break; ./

drawto_commands ::= drawto_command | drawto_command drawto_commands ;
/. case $rule_number: {
    //qDebug() << " got drawto_commands  ";
} break; ./

drawto_command ::= fakeclosepath | closepath | lineto | horizontal_lineto | vertical_lineto | curveto | smooth_curveto | quadratic_bezier_curveto | smooth_quadratic_bezier_curveto | elliptical_arc;
/. case $rule_number: {
    //qDebug() << " got drawto_command  ";
} break; ./

moveto ::= moveto_command moveto_argument_sequence ;
/. case $rule_number: {
    qDebug() << "							got moveto ";
} break; ./

moveto_argument_sequence ::= coordinate_pair | coordinate_pair comma_wsp moveto_argument_sequence | coordinate_pair moveto_argument_sequence ;
/. case $rule_number: {
    //qDebug() << " got moveto_argument_sequence ";
} break; ./

lineto ::= lineto_command lineto_argument_sequence ;
/. case $rule_number: {
    qDebug() << "							got lineto ";
} break; ./

lineto_argument_sequence ::= coordinate_pair | coordinate_pair comma_wsp lineto_argument_sequence | coordinate_pair lineto_argument_sequence ;
/. case $rule_number: {
    //qDebug() << " got lineto_argument_sequence  ";
} break; ./

horizontal_lineto ::= horizontal_lineto_command horizontal_lineto_argument_sequence ;
/. case $rule_number: {
    qDebug() << "							got horizontal_lineto ";
} break; ./

horizontal_lineto_argument_sequence ::= coordinate | coordinate comma_wsp horizontal_lineto_argument_sequence | coordinate horizontal_lineto_argument_sequence ;
/. case $rule_number: {
    //qDebug() << " got horizontal_lineto_argument_sequence ";
} break; ./

vertical_lineto ::= vertical_lineto_command vertical_lineto_argument_sequence ;
/. case $rule_number: {
    qDebug() << "							got vertical_lineto ";
} break; ./

vertical_lineto_argument_sequence ::= coordinate | coordinate comma_wsp vertical_lineto_argument_sequence | coordinate vertical_lineto_argument_sequence ;
/. case $rule_number: {
    //qDebug() << " got vertical_lineto_argument_sequence ";
} break; ./

curveto ::= curveto_command curveto_argument_sequence ;
/. case $rule_number: {
    qDebug() << "							got curveto ";
} break; ./

curveto_argument_sequence ::= curveto_argument | curveto_argument curveto_argument_sequence | curveto_argument comma_wsp curveto_argument_sequence ; 
/. case $rule_number: {
    //qDebug() << " got curveto_argument_sequence 3 ";
} break; ./  

curveto_argument ::= coordinate_pair comma_wsp coordinate_pair comma_wsp coordinate_pair | coordinate_pair comma_wsp coordinate_pair coordinate_pair | coordinate_pair coordinate_pair comma_wsp coordinate_pair | coordinate_pair coordinate_pair coordinate_pair ;
/. case $rule_number: {
    //qDebug() << " got curveto_argument ";
} break; ./

smooth_curveto ::= smooth_curveto_command smooth_curveto_argument_sequence ;
/. case $rule_number: {
    qDebug() << "							got smooth_curveto ";
} break; ./

smooth_curveto_argument_sequence ::= smooth_curveto_argument | smooth_curveto_argument smooth_curveto_argument_sequence | smooth_curveto_argument comma_wsp smooth_curveto_argument_sequence ;
/. case $rule_number: {
    //qDebug() << " got smooth_curveto_argument_sequence 3 ";
} break; ./

smooth_curveto_argument ::= coordinate_pair coordinate_pair | coordinate_pair comma_wsp coordinate_pair ;
/. case $rule_number: {
    //qDebug() << " got smooth_curveto_argument  ";
} break; ./

quadratic_bezier_curveto ::= quadratic_bezier_curveto_command quadratic_bezier_curveto_argument_sequence ;
/. case $rule_number: {
    qDebug() << "							got quadratic_bezier_curveto ";
} break; ./

quadratic_bezier_curveto_argument_sequence ::= quadratic_bezier_curveto_argument | quadratic_bezier_curveto_argument quadratic_bezier_curveto_argument_sequence | quadratic_bezier_curveto_argument comma_wsp quadratic_bezier_curveto_argument_sequence ;
/. case $rule_number: {
    //qDebug() << " got quadratic_bezier_curveto_argument ";
} break; ./

quadratic_bezier_curveto_argument ::= coordinate_pair comma_wsp coordinate_pair | coordinate_pair coordinate_pair ;
/. case $rule_number: {
    //qDebug() << " got quadratic_bezier_curveto_argument ";
} break; ./

elliptical_arc ::= elliptical_arc_command elliptical_arc_argument_sequence ;
/. case $rule_number: {
    qDebug() << "							got elliptical_arc ";
} break; ./

elliptical_arc_argument_sequence ::= elliptical_arc_argument | elliptical_arc_argument elliptical_arc_argument_sequence | elliptical_arc_argument comma_wsp elliptical_arc_argument_sequence ;
/. case $rule_number: {
    //qDebug() << " got elliptical_arc_argument_sequence ";
} break; ./

elliptical_arc_argument ::= nonnegative_number comma_wsp nonnegative_number comma_wsp number comma_wsp flag comma_wsp flag comma_wsp coordinate_pair ;
/. case $rule_number: {
    //qDebug() << " got elliptical_arc_argument ";
} break; ./

smooth_quadratic_bezier_curveto ::= smooth_quadratic_bezier_curveto_command smooth_quadratic_bezier_curveto_argument_sequence ;
/. case $rule_number: {
    qDebug() << "							got smooth_quadratic_bezier_curveto ";
} break; ./

smooth_quadratic_bezier_curveto_argument_sequence ::= coordinate_pair | coordinate_pair comma_wsp smooth_quadratic_bezier_curveto_argument_sequence | coordinate_pair smooth_quadratic_bezier_curveto_argument_sequence ; 
/. case $rule_number: {
    //qDebug() << " got smooth_quadratic_bezier_curveto_argument_sequence 3 ";
} break; ./

coordinate_pair ::= x_coordinate comma_wsp y_coordinate | x_coordinate y_coordinate ;
/. case $rule_number: {
    //qDebug() << " got coordinate_pair ";
} break; ./

x_coordinate ::= coordinate;
/. case $rule_number: {
    //qDebug() << " got x coordinate ";
} break; ./

y_coordinate ::= coordinate;
/. case $rule_number: {
    //qDebug() << " got y coordinate ";
} break; ./

comma_wsp ::= wspplus | COMMA ;
/. case $rule_number: {
    //qDebug() << " got comma_wsp 3 ";
} break; ./

wspplus ::= WHITESPACE ;
/. case $rule_number: {
    //qDebug() << " got wspplus ";
} break; ./

coordinate ::= NUMBER ;
/. 
case $rule_number: {
    //qDebug() << " got coordinate ";
    m_symStack.append(lexer->currentNumber());
} break; 
./

nonnegative_number ::= NUMBER ;
/. 
case $rule_number: {
    //qDebug() << " got nonnegative_number ";
    //not presently checking this is non-negative
    m_symStack.append(lexer->currentNumber());
} break; 
./

number ::= NUMBER ;
/. 
case $rule_number: {
    //qDebug() << " got number ";
    m_symStack.append(lexer->currentNumber());
} break; 
./

flag ::= NUMBER ;
/. 
case $rule_number: {
    //qDebug() << " got flag ";
    //not presently checking this is only 0 or 1
    m_symStack.append(lexer->currentNumber());
} break; 
./

moveto_command ::= EM ;
/. 
case $rule_number: {
    //qDebug() << "							got moveto command ";
    m_symStack.append(lexer->currentCommand());
} break; 
./

lineto_command ::= EL ;
/. 
case $rule_number: {
    //qDebug() << "							got lineto command ";
    m_symStack.append(lexer->currentCommand());
} break; 
./

horizontal_lineto_command ::= AITCH ;
/. 
case $rule_number: {
    //qDebug() << "							got horizontal_lineto command ";
    m_symStack.append(lexer->currentCommand());
} break; 
./

vertical_lineto_command ::= VEE ;
/. 
case $rule_number: {
    //qDebug() << "							got vertical_lineto command ";
    m_symStack.append(lexer->currentCommand());
} break; 
./

curveto_command ::= CEE ;
/. 
case $rule_number: {
    //qDebug() << "							got curveto command ";
    m_symStack.append(lexer->currentCommand());
} break; 
./

smooth_curveto_command ::= ESS ;
/. 
case $rule_number: {
    //qDebug() << "							got smooth curveto command ";
    m_symStack.append(lexer->currentCommand());
} break; 
./

quadratic_bezier_curveto_command ::= KYU ;
/. 
case $rule_number: {
    //qDebug() << "							got quadratic_bezier_curveto_command command ";
    m_symStack.append(lexer->currentCommand());
} break; 
./

smooth_quadratic_bezier_curveto_command ::= TEE ;
/. 
case $rule_number: {
    //qDebug() << "							got smooth_quadratic_bezier_curveto_command command ";
    m_symStack.append(lexer->currentCommand());
} break; 
./

elliptical_arc_command ::= AE ;
/. case $rule_number: {
    //qDebug() << "							got elliptical_arc_command ";
    m_symStack.append(lexer->currentCommand());
} break; ./

closepath ::= ZEE ;
/. case $rule_number: {
    qDebug() << "							got closepath ";
    m_symStack.append(lexer->currentCommand());
} break; ./


fakeclosepath ::= EKS ;
/. case $rule_number: {
    qDebug() << "							got fakeclosepath ";
} break; ./




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



























