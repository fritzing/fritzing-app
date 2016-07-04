#include <boost/version.hpp>
#if BOOST_VERSION / 100000 == 1 && BOOST_VERSION / 100 % 1000 == 54
#error "Boost 1.54 found"
#endif

int main()
{
}
