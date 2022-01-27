#define BOOST_TEST_MODULE SVG Tests
#include <boost/test/included/unit_test.hpp>

//#include <QStringList>

#define private public
#include "textutils.h"

BOOST_AUTO_TEST_CASE( test_removeFontFamilySingleQuotes )
{
	QString input1 = R"(g-text-style=“font-family:Droid Sans;text-anchor:middle;”)";
	QString input2 = R"(g-text-font-family=“Droid Sans”)";
	QString input3 = R"(style="font-family:‘Droid Sans’")";
	QString input4 = R"(style="font-family:'Droid Sans'")";
	QString input5 = R"x(<text transform="matrix(1 0 0 1 62.9226 29.5)" fill="#8C8C8C" font-family="'DroidSans'" font-size="3.5">echo</text>)x";

	bool changed;
	changed = TextUtils::removeFontFamilySingleQuotes(input1);
	BOOST_REQUIRE(!changed);

	changed = TextUtils::removeFontFamilySingleQuotes(input2);
	BOOST_REQUIRE(!changed);

	// We can probably not process opening and closing single quotes, but
	// I don't think they are valid svg anyhow.
	changed = TextUtils::removeFontFamilySingleQuotes(input3);
	BOOST_REQUIRE(!changed);

	changed = TextUtils::removeFontFamilySingleQuotes(input4);
	BOOST_REQUIRE(changed);

	changed = TextUtils::removeFontFamilySingleQuotes(input5);
	BOOST_REQUIRE(changed);
}

BOOST_AUTO_TEST_CASE( test_makeSVGHeader )
{
	std::string prefix = "<?xml version='1.0' encoding='UTF-8' standalone='no'?>\n<!-- Created with Fritzing (https://fritzing.org/) -->\n<svg xmlns:svg='http://www.w3.org/2000/svg' xmlns='http://www.w3.org/2000/svg' version='1.2' baseProfile='tiny' x='0in' y='0in' ";

	std::string suffix1 = "width='3.33333in' height='2.22222in' viewBox='0 0 3333.33 2222.22' >\n";
	QString result = TextUtils::makeSVGHeader(1000, 1000, 3333.33, 2222.22);
	BOOST_CHECK_EQUAL(result.toStdString(), prefix + suffix1);

	std::string suffix2 = "width='3.33333in' height='2.22222in' viewBox='0 0 3.33333 2.22222' >\n";
	result = TextUtils::makeSVGHeader(1000, 1, 3333.33, 2222.22);
	BOOST_CHECK_EQUAL(result.toStdString(), prefix + suffix2);

	std::string suffix3 = "width='3333.33in' height='2222.22in' viewBox='0 0 3.33333e+06 2.22222e+06' >\n";
	result = TextUtils::makeSVGHeader(1, 1000, 3333.33, 2222.22);
	BOOST_CHECK_EQUAL(result.toStdString(), prefix + suffix3);
}

BOOST_AUTO_TEST_CASE( test_optToDouble )
{
	auto result = TextUtils::optToDouble(QString("no number"));
	BOOST_REQUIRE(!result);

	auto result2 = TextUtils::optToDouble(QString("1.0"));
	BOOST_REQUIRE(result2);

	if (auto result3 = TextUtils::optToDouble(QString("nonsense"))) {
		BOOST_REQUIRE(false);
	}

	if (auto x = TextUtils::optToDouble(QString("2.0"))) {
		BOOST_REQUIRE(*x > 1.0 && *x < 3.0);
	}
}

BOOST_AUTO_TEST_CASE( test_convertToInches )
{
	auto result = TextUtils::convertToInches("no number", false);
	BOOST_REQUIRE(!result);

	auto result2 = TextUtils::convertToInches("", false);
	BOOST_REQUIRE(!result2);

	if (auto result3 = TextUtils::convertToInches("nonsense", false)) {
		BOOST_REQUIRE(false);
	}

	constexpr double epsilon = 0.00001;
	auto epsilonCheck {
		[epsilon](double value, double expected) {
			if (value > expected - epsilon && value < expected + epsilon) {
				return true;
			}
			return false;
		}
	};

	auto fromCm = TextUtils::convertToInches("10cm", false);
	BOOST_REQUIRE(fromCm);
	BOOST_REQUIRE(epsilonCheck(*fromCm, 10.0/2.54));

	BOOST_REQUIRE(epsilonCheck(*TextUtils::convertToInches("90.0", false), 1.0));
}
