#define BOOST_TEST_MODULE SVG Tests
#include <boost/test/included/unit_test.hpp>

#include "svg/svgpathparser.h"
#include "svg/svgpathlexer.h"
#include "svg/svgfilesplitter.h"
#include "svg/svgflattener.h"
#include "svg/svg2gerber.h"

#include <QTextStream>
#include <QFile>

/*
Testing that svg2gerber path2gerbCommandSlot is not influenced by newlines and whitespace.
*/

#include <algorithm>

#include <boost/lexical_cast.hpp>

BOOST_AUTO_TEST_CASE( svg2gerber_parse )
{
	QString data1 = "M1495.5,1742.5L1504.5,1742.5 M195.5,1743.5L204.5,1743.5 M1495.5,1743.5L1504.5,1743.5 M195.5,1744.5L204.5,1744.5 M1495.5,1744.5L1504.5,1744.5 M195.5,1745.5L1504.5,1745.5 M195.5,1746.5L1504.5,1746.5 M195.5,1747.5L1504.5,1747.5 M195.5,1748.5L1504.5,1748.5 M195.5,1749.5L1504.5,1749.5 \nM195.5,1750.5L1504.5,1750.5 M195.5,1751.5L1504.5,1751.5";

	QString data2 = "M1495.5,1742.5L1504.5,1742.5 M195.5,1743.5L204.5,1743.5 M1495.5,1743.5L1504.5,1743.5 M195.5,1744.5L204.5,1744.5 M1495.5,1744.5L1504.5,1744.5 M195.5,1745.5L1504.5,1745.5 M195.5,1746.5L1504.5,1746.5 M195.5,1747.5L1504.5,1747.5 M195.5,1748.5L1504.5,1748.5 M195.5,1749.5L1504.5,1749.5  M195.5,1750.5L1504.5,1750.5 M195.5,1751.5L1504.5,1751.5";

	SVG2gerber svg2gerber;
	const char * slot = SLOT(path2gerbCommandSlot(QChar, bool, QList<double> &, void *));
	PathUserData pathUserData1;
	pathUserData1.x = 0;
	pathUserData1.y = 0;
	pathUserData1.pathStarting = true;
	pathUserData1.string = "";

	PathUserData pathUserData2;
	pathUserData2.x = 0;
	pathUserData2.y = 0;
	pathUserData2.pathStarting = true;
	pathUserData2.string = "";

	SvgFlattener flattener;
	try {
		flattener.parsePath(data1, slot, pathUserData1, &svg2gerber, true);
		flattener.parsePath(data2, slot, pathUserData2, &svg2gerber, true);
	}
	catch (const QString & msg) {
	}
	catch (char const *str) {
	}
	catch (...) {
	}
	BOOST_CHECK_EQUAL(pathUserData1.string.toStdString(), pathUserData2.string.toStdString());
}
