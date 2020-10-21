/*******************************************************************
skw
Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2019 Fritzing

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

********************************************************************/

#include <QtCore>

#include <QSvgGenerator>
#include <QColor>
#include <QImageWriter>
#include <QPrinter>
#include <QSettings>
#include <QDesktopServices>
#include <QPrintDialog>
#include <QClipboard>
#include <QApplication>

#include "mainwindow.h"
#include "../debugdialog.h"
#include "../waitpushundostack.h"
#include "../help/aboutbox.h"
#include "../autoroute/autorouteprogressdialog.h"
#include "../items/virtualwire.h"
#include "../items/jumperitem.h"
#include "../items/via.h"
#include "../fsvgrenderer.h"
#include "../items/note.h"
#include "../items/partfactory.h"
#include "../eagle/fritzing2eagle.h"
#include "../sketch/breadboardsketchwidget.h"
#include "../sketch/schematicsketchwidget.h"
#include "../sketch/pcbsketchwidget.h"
#include "../partsbinpalette/binmanager/binmanager.h"
#include "../utils/expandinglabel.h"
#include "../infoview/htmlinfoview.h"
#include "../utils/bendpointaction.h"
#include "../sketch/fgraphicsscene.h"
#include "../utils/fileprogressdialog.h"
#include "../svg/svgfilesplitter.h"
#include "../version/version.h"
#include "../help/tipsandtricks.h"
#include "../dialogs/setcolordialog.h"
#include "../utils/folderutils.h"
#include "../utils/graphicsutils.h"
#include "../utils/textutils.h"
#include "../connectors/ercdata.h"
#include "../items/moduleidnames.h"
#include "../utils/zoomslider.h"
#include "../dock/layerpalette.h"
#include "../program/programwindow.h"
#include "../utils/autoclosemessagebox.h"
#include "../svg/gerbergenerator.h"
#include "../processeventblocker.h"

#include "../simulation/ngspice.h"
#include "../simulation/spice_simulator.h"
#include "../simulation/sim_types.h"

#if defined(__MINGW32__) || defined(_MSC_VER)
#include <windows.h>  // for Sleep
#else
#include <unistd.h>
#include <ctype.h>
#endif
#include <iostream>

////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////

void MainWindow::simulate()
{
	static std::shared_ptr<SPICE_SIMULATOR> simulator = SPICE_SIMULATOR::CreateInstance( "ngspice" );
	if( !simulator )
	{
		throw std::runtime_error( "Could not create simulator instance" );
		return;
	}
	QList< QList<ConnectorItem *>* > netList;
	QSet<ItemBase *> itemBases;
	QString spiceNetlist = getSpiceNetlist("Simulator Netlist", netList, itemBases);
	std::cout << "Netlist: " << spiceNetlist.toStdString() << std::endl;
	std::cout << "-----------------------------------" <<std::endl;
	simulator->Command("remcirc");
	std::cout << "-----------------------------------" <<std::endl;
	simulator->Init();
	std::cout << "-----------------------------------" <<std::endl;
	std::string netlist = "Test simulator\nV1 1 0 5\nR1 1 2 100\nR2 2 0 100\n.OP\n.END";
	simulator->LoadNetlist( spiceNetlist.toStdString());
	std::cout << "-----------------------------------" <<std::endl;
	simulator->Command("listing");
	std::cout << "-----------------------------------" <<std::endl;
	simulator->Run();
	std::cout << "-----------------------------------" <<std::endl;
#if defined(__MINGW32__) || defined(_MSC_VER)
	Sleep(100);
#else
	usleep(100000);
#endif


	QString command = QString("print v(2)");
	simulator->Command( command.toStdString() );
	std::cout << "-----------------------------------" <<std::endl;
	double volt = simulator->GetDataPoint("v(2)");
	std::cout << "-----------------------------------" <<std::endl;
	std::cout << "voltage: " <<volt<<std::endl;
	std::cout << "-----------------------------------" <<std::endl;
	simulator->Stop();
	std::cout << "-----------------------------------" <<std::endl;

}
