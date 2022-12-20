#define BOOST_TEST_MODULE PROJECT_PROPERTIES Tests
#include <boost/test/included/unit_test.hpp>

#include "project_properties.h"

#include <QDomDocument>

/*
Testing project_properties.cpp, a class for encapsulating project specific properties/settings.
*/

#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE( project_properties_simple )
{
	ProjectProperties projectProperties;

	BOOST_CHECK_EQUAL(projectProperties.getPartLabelFontCutoffCorrectionFlag(), true);
	BOOST_CHECK_EQUAL(projectProperties.getProjectProperty(ProjectPropertyKeyPartLabelFontCutoffCorrection).toStdString(), "yes");

	QDomElement dummy;
	projectProperties.load(dummy);

	BOOST_CHECK_EQUAL(projectProperties.getPartLabelFontCutoffCorrectionFlag(), false);
	BOOST_CHECK_EQUAL(projectProperties.getProjectProperty(ProjectPropertyKeyPartLabelFontCutoffCorrection).toStdString(), "no");
}

BOOST_AUTO_TEST_CASE( project_properties_xml_old_project_resaved )
{
	ProjectProperties projectProperties;

	QString xml = QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?> <project_properties> <part_label_font_cutoff_correction value=\"no\"/> <pcb_part_label_font value=\"OCRA\"/> </project_properties>");
	QDomDocument doc;
	doc.setContent(xml, true);
	projectProperties.load(doc.documentElement());

	BOOST_CHECK_EQUAL(projectProperties.getPartLabelFontCutoffCorrectionFlag(), false);
	BOOST_CHECK_EQUAL(projectProperties.getProjectProperty(ProjectPropertyKeyPartLabelFontCutoffCorrection).toStdString(), "no");
	BOOST_CHECK_EQUAL(projectProperties.getProjectProperty(ProjectPropertyKeyPcbPartLabelFont).toStdString(), "OCRA");
}

BOOST_AUTO_TEST_CASE( project_properties_xml_empty )
{
	ProjectProperties projectProperties;

	QString xml = QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	QDomDocument doc;
	doc.setContent(xml, true);
	projectProperties.load(doc.documentElement());

	BOOST_CHECK_EQUAL(projectProperties.getPartLabelFontCutoffCorrectionFlag(), false);
	BOOST_CHECK_EQUAL(projectProperties.getProjectProperty(ProjectPropertyKeyPartLabelFontCutoffCorrection).toStdString(), "no");
	BOOST_CHECK_EQUAL(projectProperties.getProjectProperty(ProjectPropertyKeyPcbPartLabelFont).toStdString(), "OCRA");
}
BOOST_AUTO_TEST_CASE( project_properties_xml_no_prop )
{
	ProjectProperties projectProperties;

	QString xml = QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?> <project_properties> </project_properties>");
	QDomDocument doc;
	doc.setContent(xml, true);
	projectProperties.load(doc.documentElement());

	BOOST_CHECK_EQUAL(projectProperties.getPartLabelFontCutoffCorrectionFlag(), false);
	BOOST_CHECK_EQUAL(projectProperties.getProjectProperty(ProjectPropertyKeyPartLabelFontCutoffCorrection).toStdString(), "no");
	BOOST_CHECK_EQUAL(projectProperties.getProjectProperty(ProjectPropertyKeyPcbPartLabelFont).toStdString(), "OCRA");
}

BOOST_AUTO_TEST_CASE( project_properties_xml_new_project_saved )
{
	ProjectProperties projectProperties;

	QString xml = QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><project_properties><part_label_font_cutoff_correction value=\"yes\"/><pcb_part_label_font value=\"OCR-Fritzing-mono\"/></project_properties>");
	QDomDocument doc;
	doc.setContent(xml, true);
	projectProperties.load(doc.documentElement());

	BOOST_CHECK_EQUAL(projectProperties.getPartLabelFontCutoffCorrectionFlag(), true);
	BOOST_CHECK_EQUAL(projectProperties.getProjectProperty(ProjectPropertyKeyPartLabelFontCutoffCorrection).toStdString(), "yes");
	BOOST_CHECK_EQUAL(projectProperties.getProjectProperty(ProjectPropertyKeyPcbPartLabelFont).toStdString(), "OCR-Fritzing-mono");
}

BOOST_AUTO_TEST_CASE( project_properties_xml_somefont )
{
	ProjectProperties projectProperties;

	QString xml = QString("<?xml version=\"1.0\" encoding=\"UTF-8\"?><project_properties><part_label_font_cutoff_correction value=\"yes\"/><pcb_part_label_font value=\"Somefont\"/></project_properties>");
	QDomDocument doc;
	doc.setContent(xml, true);
	projectProperties.load(doc.documentElement());

	BOOST_CHECK_EQUAL(projectProperties.getPartLabelFontCutoffCorrectionFlag(), true);
	BOOST_CHECK_EQUAL(projectProperties.getProjectProperty(ProjectPropertyKeyPartLabelFontCutoffCorrection).toStdString(), "yes");
	BOOST_CHECK_EQUAL(projectProperties.getProjectProperty(ProjectPropertyKeyPcbPartLabelFont).toStdString(), "Somefont");
}

BOOST_AUTO_TEST_CASE( project_properties_xml_save_load )
{
	ProjectProperties projectProperties;

	QString xmlWriteTo;
	QXmlStreamWriter streamWriter(&xmlWriteTo);
	projectProperties.saveProperties(streamWriter);

	QDomDocument doc;
	doc.setContent(xmlWriteTo, true);
	projectProperties.load(doc.documentElement());

	BOOST_CHECK_EQUAL(projectProperties.getPartLabelFontCutoffCorrectionFlag(), true);
	BOOST_CHECK_EQUAL(projectProperties.getProjectProperty(ProjectPropertyKeyPartLabelFontCutoffCorrection).toStdString(), "yes");
	BOOST_CHECK_EQUAL(projectProperties.getProjectProperty(ProjectPropertyKeyPcbPartLabelFont).toStdString(), "OCR-Fritzing-mono");
}
