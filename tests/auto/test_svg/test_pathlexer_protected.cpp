#include "svg/svgpathgrammar_p.h"

/*
Test how SVGPathLexer::lexer cleans up (pre-processes) various svg path element
data strings

Testing that needs access to protected members of the class under test. This
is not the best way to do this, but it is simple, and is fine for the
functionality cases. Can break if class member allocation order is important
*/

// Get access to protected members of class SVGPathLexer for testing
#define protected public
#include "svg/svgpathlexer.h"
#undef protected

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE( pathlexer_clean )
{
	const QStringList test_cases = {
		"m0,0",
		"m 0 ,  0 \t\n   ",
		"m0,0x",
		"m 0 , 0 x",
		"m 0 0\nx\n",
		"m0,0a 2.6,2.6 0 0 1 5.2,0v5.2a 2.6, 2.6 0 0 1-5.2,0 z "
			"m 0.5,3 a 1,\t 1\n   0 0 0 4.2,0v-0.8a 1,  1   0 0 0 -4.2,0z  "
	};
	const QStringList expected_results = {
		"m0,0",
		"m0,0",
		"m0,0x",
		"m0,0x",
		"m0 0x",
		"m0,0a2.6,2.6 0 0 1 5.2,0v5.2a2.6,2.6 0 0 1 -5.2,0z"
			"m0.5,3a1,1 0 0 0 4.2,0v-0.8a1,1 0 0 0 -4.2,0z"
	};
	for (int i = 0; i < test_cases.size(); ++i) {
		BOOST_CHECK_NO_THROW({
			QString dataCopy(test_cases.at(i));
			SVGPathLexer lexer(dataCopy);
			if (lexer.m_source != expected_results.at(i)) {
				BOOST_ERROR("check lexer.m_source.toStdString() == expected_results.at("
					<< i << ").toStdString() has failed [\n\"" << lexer.m_source.toStdString()
					<< "\" != \n\"" << expected_results.at(i).toStdString() << "\"]");
			}
		});
	}
}
