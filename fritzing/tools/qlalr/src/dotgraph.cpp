/****************************************************************************
**
** Copyright (C) 2007-2007 Trolltech ASA. All rights reserved.
**
** This file is part of the QLALR project on Trolltech Labs.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://www.trolltech.com/products/qt/licensing.html or contact the
** sales department at sales@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/


#include <QtCore/QTextStream>

#include "lalr.h"
#include "dotgraph.h"

DotGraph::DotGraph(QTextStream &o):
  out (o)
{
}

void DotGraph::operator () (Automaton *aut)
{
  Grammar *g = aut->_M_grammar;

  out << "digraph {" << endl << endl;

  out << "subgraph Includes {" << endl;
  for (Automaton::IncludesGraph::iterator incl = Automaton::IncludesGraph::begin_nodes ();
       incl != Automaton::IncludesGraph::end_nodes (); ++incl)
    {
      for (Automaton::IncludesGraph::edge_iterator edge = incl->begin (); edge != incl->end (); ++edge)
        {
          out << "\t\"(" << aut->id (incl->data.state) << ", " << incl->data.nt << ")\"";
          out << "\t->\t";
          out << "\"(" << aut->id ((*edge)->data.state) << ", " << (*edge)->data.nt << ")\"\t";
          out << "[label=\"" << incl->data.state->follows [incl->data.nt] << "\"]";
          out << endl;
        }
    }
  out << "}" << endl << endl;


  out << "subgraph LRA {" << endl;
  //out << "node [shape=record];" << endl << endl;

  for (StatePointer q = aut->states.begin (); q != aut->states.end (); ++q)
    {
      int state = aut->id (q);

      out << "\t" << state << "\t[shape=record,label=\"{";

      out << "<0> State " << state;

      int index = 1;
      for (ItemPointer item = q->kernel.begin (); item != q->kernel.end (); ++item)
        out << "| <" << index++ << "> " << *item;

      out << "}\"]" << endl;

      for (Bundle::iterator a = q->bundle.begin (); a != q->bundle.end (); ++a)
        {
          const char *clr = g->isTerminal (a.key ()) ? "blue" : "red";
          out << "\t" << state << "\t->\t" << aut->id (*a) << "\t[color=\"" << clr << "\",label=\"" << a.key () << "\"]" << endl;
        }
      out << endl;
    }

  out << "}" << endl;
  out << endl << endl << "}" << endl;
}
