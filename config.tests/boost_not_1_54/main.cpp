#include <boost/version.hpp>

#define BOOST_PATCH_LEVEL   (BOOST_VERSION % 100)
#define BOOST_MINOR_VERSION (BOOST_VERSION / 100 % 1000)
#define BOOST_MAJOR_VERSION (BOOST_VERSION / 100000)

#if ((BOOST_MAJOR_VERSION == 1) && (BOOST_MINOR_VERSION == 54))
#  error "Boost 1.54 found"
#endif

int main() {}
