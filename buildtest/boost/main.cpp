#define BOOST_TEST_MODULE FritzingBoostTest
#include <boost/test/included/unit_test.hpp>
#include <boost/version.hpp>

BOOST_AUTO_TEST_CASE( boost_version )
{
    BOOST_REQUIRE_NE( BOOST_LIB_VERSION, "1_54");
}
