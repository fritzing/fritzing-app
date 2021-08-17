#define BOOST_TEST_MODULE SVG Tests
#include <boost/test/included/unit_test.hpp>

#include "svg/svg2gerber.h"
#include "utils/textutils.h"

/*
Testing how SVGPathParser::parser handles various valid svg path element
specifcation strings, and segments
*/

#include <algorithm>

#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>

#include <QFile>
#include <QTextStream>

BOOST_AUTO_TEST_CASE( test_svg2gerber )
{
	const QStringList svgs = {
		"<g transform='translate(825.367,394.456),matrix(1, 0, 0, 1, 49.3696, 86.397)'> <path stroke='black' stroke-width='11.1082' id='0' fill='none' d='M166.622,5.55409L1277.44,5.55409L1277.44,1394.08Z'/> </g> </svg>", // triangle
	};
	const QStringList gerbers = {
		"G04 MADE WITH FRITZING*\nG04 WWW.FRITZING.ORG*\nG04 DOUBLE SIDED*\nG04 HOLES PLATED*\nG04 CONTOUR ON CENTER OF CONTOUR VECTOR*\n%ASAXBY*%\n%FSLAX23Y23*%\n%MOIN*%\n%OFA0B0*%\n%SFA1.0B1.0*%\n%ADD10C,0.011108*%\n%LNSILK1*%\nG90*\nG70*\nG54D10*\nX167Y2217D02*\nX1277Y2217D01*\nX1277Y828D01*\nD02*\nG04 End of Silk1*\nM02*",
	};

	SVG2gerber gerber;
	QString header = TextUtils::makeSVGHeader(1000, 1000, 3333.33, 2222.22);
	gerber.convert(header + svgs[0], 2, "Silk1", SVG2gerber::ForSilk, QSizeF(3333.33, 2222.22));
	BOOST_CHECK_EQUAL(gerber.getGerber().toStdString(), gerbers[0].toStdString());
}
