/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2012 Fachhochschule Potsdam - http://fh-potsdam.de

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

$Revision: 6565 $:
$Author: irascibl@gmail.com $:
$Date: 2012-10-15 12:10:48 +0200 (Mo, 15. Okt 2012) $

********************************************************************/

/*
 * Class that manage all the interactive helps in the main window
 */

#ifndef HELPER_H_
#define HELPER_H_

#include <QObject>

#include "sketchmainhelp.h"
#include "../mainwindow/mainwindow.h"

class Helper : public QObject {
	Q_OBJECT
	
	public:
		Helper(MainWindow *owner, bool doShow);
		~Helper();

		void toggleHelpVisibility(int ix);
		void setHelpVisibility(int index, bool show);
		bool helpVisible(int index);

	public:
		static void initText();

	protected slots:
		void somethingDroppedIntoView(const QPoint &);
		void viewSwitched();

	protected:
		void connectToView(SketchWidget* view);
		SketchMainHelp *helpForIndex(int index);

	protected:
		MainWindow *m_owner;

		SketchMainHelp *m_breadMainHelp;
		SketchMainHelp *m_schemMainHelp;
		SketchMainHelp *m_pcbMainHelp;

		bool m_stillWaitingFirstDrop;
		bool m_stillWaitingFirstViewSwitch;

		double m_prevVScroolW;
		double m_prevHScroolH;

	protected:
		static QString BreadboardHelpText;
		static QString SchematicHelpText;
		static QString PCBHelpText;

};

#endif /* HELPER_H_ */
