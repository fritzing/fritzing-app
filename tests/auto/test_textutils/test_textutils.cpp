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
