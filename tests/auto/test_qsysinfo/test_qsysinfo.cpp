#define BOOST_TEST_MODULE QSYSINFO Tests
#include <boost/test/included/unit_test.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>

#include <iostream>

#include <QThread>
#include <QSysInfo>
#include <QUrl>
#include <QtGlobal>

BOOST_AUTO_TEST_CASE( qsysinfo )
{
	QString siVersion(QUrl::toPercentEncoding("1.0.0"));
	QString siSystemName(QUrl::toPercentEncoding(QSysInfo::productType()));
	QString siSystemVersion(QUrl::toPercentEncoding(QSysInfo::productVersion()));
	QString siKernelName(QUrl::toPercentEncoding(QSysInfo::kernelType()));
	QString siKernelVersion(QUrl::toPercentEncoding(QSysInfo::kernelVersion()));
	QString siArchitecture(QUrl::toPercentEncoding(QSysInfo::currentCpuArchitecture()));
	QString string = QString("?version=%2&sysname=%3&kernname=%4&kernversion=%5&arch=%6&sysversion=%7")
	                 .arg(siVersion)
	                 .arg(siSystemName)
	                 .arg(siKernelName)
	                 .arg(siKernelVersion)
	                 .arg(siArchitecture)
	                 .arg(siSystemVersion)
	                 ;

	std::cout << std::endl;
	std::cout << string.toStdString() << std::endl;

#ifdef Q_OS_LINUX
	QString result("?version=1.0.0&sysname=ubuntu&kernname=linux&kernversion=5.11.0-38-generic&arch=x86_64&sysversion=20.04");
	BOOST_CHECK_EQUAL(string.toStdString(), result.toStdString());
#endif
}
