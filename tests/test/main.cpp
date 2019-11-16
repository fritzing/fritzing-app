#define BOOST_TEST_MODULE MyTest
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE( my_test )
{
    my_class test_object( "qwerty" );

    BOOST_CHECK( test_object.is_valid() );
}