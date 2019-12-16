#define BOOST_TEST_MODULE SVG Tests
#include <boost/test/included/unit_test.hpp>

//#include <QStringList>

#define private public
#include "svg/svgpathregex.h"

BOOST_AUTO_TEST_CASE( mfinder )
{
	QString input1 = "M11488.1,3614.03L78.7354,3614.03C35.1135,3614.03,0,3578.92,0,3535.3L0,78.7298C0,35.1078,35.1135";
	MFinder.indexIn(input1.trimmed());
	QString priorM = MFinder.cap(1) + MFinder.cap(2) + "," + MFinder.cap(3) + " ";
	BOOST_REQUIRE_EQUAL(priorM.toStdString(), std::string("M11488.1,3614.03 "));

	QString input2 = "m-800.008,0 m-924.988,0 m-2550.01,0L7094.74,243.586l-133.688,0l0,-57.5969l-199.138,0l0,57.5969l-133.708,0l0,232.279z";
	MFinder.indexIn(input2.trimmed());
	priorM = MFinder.cap(1) + MFinder.cap(2) + "," + MFinder.cap(3) + " ";
	BOOST_REQUIRE_EQUAL(priorM.toStdString(), std::string("m-800.008,0 "));

	// FIXME
	// We currently ingore this kind of malformed svg path and pass on an single ", "
	QString input3 = "m,924.988,0L3644.74,243.586l-133.688,0l0,-57.5969l-199.138,0l0,57.5969l-133.708,0l0,232.279z";
	MFinder.indexIn(input3.trimmed());
	priorM = MFinder.cap(1) + MFinder.cap(2) + "," + MFinder.cap(3) + " ";
	BOOST_REQUIRE_EQUAL(priorM.toStdString(), std::string(", "));
}
