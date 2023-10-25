#define BOOST_TEST_MODULE PROJECT_PROPERTIES Tests
#include <boost/test/included/unit_test.hpp>

#include "project_properties.h"

#include <QDomDocument>

/*
Testing project_properties.cpp, a class for encapsulating project specific properties/settings.
*/

#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE( project_properties_xml_empty )
{
	ProjectProperties projectProperties;

	QString xml = QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	QDomDocument doc;
	doc.setContent(xml, true);
	projectProperties.load(doc.documentElement());

	BOOST_CHECK_EQUAL(projectProperties.getProjectProperty("invalid").toStdString(), "no");
}

BOOST_AUTO_TEST_CASE( project_properties_xml_no_prop )
{
	ProjectProperties projectProperties;

	QString xml = QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?> <project_properties> </project_properties>");
	QDomDocument doc;
	doc.setContent(xml, true);
	projectProperties.load(doc.documentElement());

	BOOST_CHECK_EQUAL(projectProperties.getProjectProperty("invalid").toStdString(), "no");
}


BOOST_AUTO_TEST_CASE( project_properties_xml_somefont )
{
	ProjectProperties projectProperties;

	QString xml = QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><project_properties><test1 value=\"yes\"/><test2 value=\"Somefont\"/></project_properties>");
	QDomDocument doc;
	doc.setContent(xml, true);
	projectProperties.load(doc.documentElement());

	BOOST_CHECK_EQUAL(projectProperties.getProjectProperty("test1").toStdString(), "yes");
	BOOST_CHECK_EQUAL(projectProperties.getProjectProperty("test2").toStdString(), "Somefont");
}
