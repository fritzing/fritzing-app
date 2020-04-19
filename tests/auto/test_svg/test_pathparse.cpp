#include "svg/svgpathparser.h"
#include "svg/svgpathlexer.h"

/*
Testing how SVGPathParser::parser handles various valid svg path element
specifcation strings, and segments
*/

#include <algorithm>

#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE( pathparse_good )
{
	// reference: https://www.w3.org/TR/SVG/paths.html
	const QStringList goodInputs = {
		"m0,0x", // 0
		"m5,9.9x", // 1 standard comma separated coordinates
		"m-5,-9.9x", // 2 same for negative values
		"m-4 -9.8x", // 3 whitespace instead of comma separator
		"m-3-9.7x", // 4 no separator when 2nd value is negative
		"m0,0z", // 5 'real' close marker
		"m1,-2a2.6,3.5,0,0,1,-5.2,0x", // 6 all comma separators
		"m2 -2a2.6 3.5 0 0 1 -5.2 0x", // 7 all whitespace separators
		"m3-2a2.6 3.5 0 0 1-5.2 0x", // 8 no separator before negative value
		"m4-2a2.6-3.5 0 0 1-5.2 0x", // 9 no separator before negative value
		"m-2+9.7x", // 10 "+" separator before positive value
	};
	const QList<QList<QVariant>> expectedStacks = {
		{ QChar('m'), double(0), double(0) }, // "m0,0x" // 0
		{ QChar('m'), double(5), double(9.9) }, // "m5,9.9x" // 1
		{ QChar('m'), double(-5), double(-9.9) }, // "m-5,-9.9x" // 2
		{ QChar('m'), double(-4), double(-9.8) }, // "m-4 -9.8x" // 3
		{ QChar('m'), double(-3), double(-9.7) }, // "m-3-9.7x" // 4
		{ QChar('m'), double(0), double(0), QChar('z') }, // "m0,0z" // 5
		{ QChar('m'), double(1), double(-2), // "m1,-2" // 6
		  QChar('a'), double(2.6), double(3.5), double(0), double(0), double(1), double(-5.2), double(0) }, // "a2.6,3.5,0,0,1,-5.2,0x"
		{ QChar('m'), double(2), double(-2), // "m2 -2" // 7
		  QChar('a'), double(2.6), double(3.5), double(0), double(0), double(1), double(-5.2), double(0) }, // "a2.6 3.5 0 0 1 -5.2 0x"
		{ QChar('m'), double(3), double(-2), // "m3-2" // 8
		  QChar('a'), double(2.6), double(3.5), double(0), double(0), double(1), double(-5.2), double(0) }, // "a2.6 3.5 0 0 1-5.2 0x"
		{ QChar('m'), double(4), double(-2), // "m4-2" // 8
		  QChar('a'), double(2.6), double(-3.5), double(0), double(0), double(1), double(-5.2), double(0) }, // "a2.6-3.5 0 0 1-5.2 0x"
		{ QChar('m'), double(-2), double(+9.7) }, // "m-2+9.7x" // 10
	};

	for (int inp = 0; inp < goodInputs.size(); ++inp) { // Sequence through the sample strings
		QList<QVariant> goodResult = expectedStacks.at(inp);
		SVGPathParser parser; // Create a new parser instance each time
		BOOST_CHECK_NO_THROW({ // Nothing here is ever supposed to raise an exception
			QString dataCopy(goodInputs.at(inp));
			SVGPathLexer lexer(dataCopy);
			int parseResult = parser.parse(lexer);
			if (!parseResult) {
				BOOST_ERROR("for input " << inp << ", check parser.parse(lexer) has failed");
			}
		});
		// Detailed comparison of the parsed stack with what is expected for the input
		QVector<QVariant> parseStack = parser.symStack();
		if (parseStack.size() != goodResult.size()) { // The stack size better match
			BOOST_ERROR("for input " << inp
				<< ", check parseStack.size() == goodResult.size() has failed ["
				<< parseStack.size() << " != " << goodResult.size() << "]");
		}
		for (int entry = 0; entry < std::min(parseStack.size(), goodResult.size()); entry++) {
			// Every entry on the stack should match
			QVariant parseEntry = parseStack.at(entry);
			QVariant goodEntry = goodResult.at(entry);
			// if (parseEntry != goodEntry) { // Will compare equal if one is int, other double, with matching value
			if (parseEntry.type() != goodEntry.type() || parseEntry != goodEntry) {
				BOOST_ERROR("for input " << inp
					<< ", check parseStack.at(" << entry << ") == goodResult.at(" << entry << ") has failed ["
					<< "<" << parseEntry.typeName() << ">"
					<< ((QMetaType::Type)parseEntry.type() == QMetaType::QChar
						? boost::lexical_cast<std::string>(parseEntry.toChar().toLatin1())
						: boost::lexical_cast<std::string>(parseEntry.toDouble()))
					<< " != <" << goodEntry.typeName() << ">"
					<< ((QMetaType::Type)goodEntry.type() == QMetaType::QChar
						? boost::lexical_cast<std::string>(goodEntry.toChar().toLatin1())
						: boost::lexical_cast<std::string>(goodEntry.toDouble()))
				);
			}
		}
	}
}
