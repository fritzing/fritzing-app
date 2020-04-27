#include "svg/svgpathgrammar_p.h"
#include "svg/svgpathlexer.h"

/*
Test how SVGPathLexer::lexer processes various svg path element data strings
*/

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE( pathlexer_tree )
{
	const QStringList inputs = {
		"m0,0",
		"m0,0z",
		"m0,0x",
		"m 0 , 0 x",
		"m 0 0\nx\n",
		"m0,0a 2.6,2.6 0 0 1 5.2,0v5.2a 2.6,2.6 0 0 1-5.2,0z"
			"m 0.5,3a 1,  1   0 0 0 4.2,0v-0.8a 1,  1   0 0 0-4.2,0z  ",
		"m0,0a 2.6,2.6 0 0 1 5.2,0v5.2a 2.6,2.6 0 0 1 -5.2,0z\n  "
			"m 0.5,3a 1,  1   0 0 0 4.2,0v-0.8a 1,  1   0 0 0-4.2,0z  "
	};
	const QList<QString> expectedCommands = {
		"mmmmm", // "m0,0"
		"mmmmzz", // "m0,0z"
		"mmmmxx", // "m0,0x"
		"mmmmxx", // "m 0 , 0 x"
		"mmmmxx", // "m 0 0\nx\n"
		"mmmm" // m0,0
			"aaaaaaaaaaaaaa" // a 2.6,2.6 0 0 1 5.2,0
			"vv" // v5.2
			"aaaaaaaaaaaaaa" // a 2.6,2.6 0 0 1 -5.2,0
			"z" // z
			"mmmm" // m 0.5,3
			"aaaaaaaaaaaaaa" // a 1,  1   0 0 0 4.2,0
			"vv" // v-0.8
			"aaaaaaaaaaaaaa" // a 1,  1   0 0 0 -4.2,0
			"zz", // z  "
		"mmmm" // m0,0
			"aaaaaaaaaaaaaa" // a 2.6,2.6 0 0 1 5.2,0
			"vv" // v5.2
			"aaaaaaaaaaaaaa" // a 2.6,2.6 0 0 1 -5.2,0
			"z" // z\n  "
			"mmmm" // m 0.5,3
			"aaaaaaaaaaaaaa" // a 1,  1   0 0 0 4.2,0
			"vv" // v-0.8
			"aaaaaaaaaaaaaa" // a 1,  1   0 0 0 -4.2,0
			"zz", // z  "
	};
	const QList<QList<double>> expectedNumbers = {
		{ 0.0, 0.0, 0.0, 0.0, 0.0 }, // "m0,0"
		{ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 }, // "m0,0z"
		{ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 }, // "m0,0x"
		{ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 }, // "m 0 , 0 x"
		{ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 }, // "m 0 0\nx\n"
		{ 0.0, 0.0, 0.0, 0.0, // m0,0
			0.0, 2.6, 2.6, 2.6, 2.6, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 5.2, 5.2, 0.0, // a 2.6,2.6 0 0 1 5.2,0
			0.0, 5.2, // v5.2
			5.2, 2.6, 2.6, 2.6, 2.6, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, -5.2, -5.2, 0.0, // a 2.6,2.6 0 0 1-5.2,0
			0.0, // z
			0.0, 0.5, 0.5, 3.0, // m 0.5,3
			3.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 4.2, 4.2, 0.0, // a 1,  1   0 0 0 4.2,0
			0.0, -0.8, // v-0.8
			-0.8, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -4.2, -4.2, 0.0, // a 1,  1   0 0 0-4.2,0
			0.0, 0.0 }, // "z  "
		{ 0.0, 0.0, 0.0, 0.0, // m0,0
			0.0, 2.6, 2.6, 2.6, 2.6, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 5.2, 5.2, 0.0, // a 2.6,2.6 0 0 1 5.2,0
			0.0, 5.2, // v5.2
			5.2, 2.6, 2.6, 2.6, 2.6, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, -5.2, -5.2, 0.0, // a 2.6,2.6 0 0 1 -5.2,0
			0.0, // z
			0.0, 0.5, 0.5, 3.0, // m 0.5,3
			3.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 4.2, 4.2, 0.0, // a 1,  1   0 0 0 4.2,0
			0.0, -0.8, // v-0.8
			-0.8, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -4.2, -4.2, 0.0, // a 1,  1   0 0 0-4.2,0
			0.0, 0.0 }, // z
	};
	const QList<QList<int>> expectedResults = {
		{ SVGPathGrammar::EM, SVGPathGrammar::NUMBER, SVGPathGrammar::COMMA, SVGPathGrammar::NUMBER,
		  SVGPathGrammar::EOF_SYMBOL // "m0,0"
		},
		{ SVGPathGrammar::EM, SVGPathGrammar::NUMBER, SVGPathGrammar::COMMA, SVGPathGrammar::NUMBER,
		  SVGPathGrammar::ZEE, SVGPathGrammar::EOF_SYMBOL // "m0,0z"
		},
		{ SVGPathGrammar::EM, SVGPathGrammar::NUMBER, SVGPathGrammar::COMMA, SVGPathGrammar::NUMBER,
		  SVGPathGrammar::EKS, SVGPathGrammar::EOF_SYMBOL // "m0,0x"
		},
		{ SVGPathGrammar::EM, SVGPathGrammar::NUMBER, SVGPathGrammar::COMMA, SVGPathGrammar::NUMBER,
		  SVGPathGrammar::EKS, SVGPathGrammar::EOF_SYMBOL // "m 0 , 0 x"
		},
		{ SVGPathGrammar::EM, SVGPathGrammar::NUMBER, SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER,
		  SVGPathGrammar::EKS, SVGPathGrammar::EOF_SYMBOL // "m 0 0\nx\n"
		},
		{ SVGPathGrammar::EM, SVGPathGrammar::NUMBER, SVGPathGrammar::COMMA, SVGPathGrammar::NUMBER, /* 3 */
		  SVGPathGrammar::AE, SVGPathGrammar::NUMBER, SVGPathGrammar::COMMA, SVGPathGrammar::NUMBER, /* 7 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 9 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 11 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 13 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 15 */
			SVGPathGrammar::COMMA, SVGPathGrammar::NUMBER, /* 17 */
		  SVGPathGrammar::VEE, SVGPathGrammar::NUMBER, /* 19 */
		  SVGPathGrammar::AE, SVGPathGrammar::NUMBER, SVGPathGrammar::COMMA, SVGPathGrammar::NUMBER, /* 23 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 25 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 27 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 29 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 31 */
			SVGPathGrammar::COMMA, SVGPathGrammar::NUMBER, /* 33 */
		  SVGPathGrammar::ZEE, /* 34 */
		  SVGPathGrammar::EM, SVGPathGrammar::NUMBER, SVGPathGrammar::COMMA, SVGPathGrammar::NUMBER, /* 38 */
		  SVGPathGrammar::AE, SVGPathGrammar::NUMBER, SVGPathGrammar::COMMA, SVGPathGrammar::NUMBER, /* 42 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 44 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 46 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 48 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 50 */
			SVGPathGrammar::COMMA, SVGPathGrammar::NUMBER, /* 52 */
		  SVGPathGrammar::VEE, SVGPathGrammar::NUMBER, /* 54 */
		  SVGPathGrammar::AE, SVGPathGrammar::NUMBER, SVGPathGrammar::COMMA, SVGPathGrammar::NUMBER, /* 58 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 60 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 62 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 64 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 66 */
			SVGPathGrammar::COMMA, SVGPathGrammar::NUMBER, /* 68 */
		  SVGPathGrammar::ZEE, /* 69 */
		  SVGPathGrammar::EOF_SYMBOL /* 70 */
		},
		{ SVGPathGrammar::EM, SVGPathGrammar::NUMBER, SVGPathGrammar::COMMA, SVGPathGrammar::NUMBER, /* 3 */
		  SVGPathGrammar::AE, SVGPathGrammar::NUMBER, SVGPathGrammar::COMMA, SVGPathGrammar::NUMBER, /* 7 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 9 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 11 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER,  /* 13 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 15 */
			SVGPathGrammar::COMMA, SVGPathGrammar::NUMBER, /* 17 */
		  SVGPathGrammar::VEE, SVGPathGrammar::NUMBER, /* 19 */
		  SVGPathGrammar::AE, SVGPathGrammar::NUMBER, SVGPathGrammar::COMMA, SVGPathGrammar::NUMBER, /* 23 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 25 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 27 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 29 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 31 */
			SVGPathGrammar::COMMA, SVGPathGrammar::NUMBER, /* 33 */
		  SVGPathGrammar::ZEE, /* 34 */
		  SVGPathGrammar::EM, SVGPathGrammar::NUMBER, SVGPathGrammar::COMMA, SVGPathGrammar::NUMBER, /* 38 */
		  SVGPathGrammar::AE, SVGPathGrammar::NUMBER, SVGPathGrammar::COMMA, SVGPathGrammar::NUMBER, /* 42 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 44 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 46 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 48 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 50 */
			SVGPathGrammar::COMMA, SVGPathGrammar::NUMBER, /* 52 */
		  SVGPathGrammar::VEE, SVGPathGrammar::NUMBER, /* 54 */
		  SVGPathGrammar::AE, SVGPathGrammar::NUMBER, SVGPathGrammar::COMMA, SVGPathGrammar::NUMBER, /* 58 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 60 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 62 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, /* 64 */
			SVGPathGrammar::WHITESPACE, SVGPathGrammar::NUMBER, SVGPathGrammar::COMMA, SVGPathGrammar::NUMBER, /* 68 */
		  SVGPathGrammar::ZEE, /* 69 */
		  SVGPathGrammar::EOF_SYMBOL /* 70 */
		}
	};

	for (int inp = 0; inp < inputs.size(); ++inp) {
		QString dataCopy(inputs.at(inp));
		SVGPathLexer lexer(dataCopy);
		QList<int> inputResults = expectedResults.at(inp);
		QString inputCommands = expectedCommands.at(inp);
		QList<double> inputNumbers = expectedNumbers.at(inp);
		for (int step = 0; step < inputResults.size(); step++) {
			BOOST_CHECK_NO_THROW({
				int lexerResult = lexer.lex();
				if (lexerResult != inputResults.at(step)) {
					BOOST_ERROR("for input " << inp
						<< ", check lexer.lex() == inputResults.at(" << step << ") has failed ["
						<< lexerResult << " != " << inputResults.at(step) << "]");
				}
				QChar currentCommand = lexer.currentCommand();
				if (currentCommand != inputCommands.at(step)) {
					BOOST_ERROR("for input " << inp
						<< ", check lexer.currentCommand() == inputCommands.at(" << step << ") has failed ["
						<< currentCommand.toLatin1() << " != " << inputCommands.at(step).toLatin1() << "]"
					);
				}
				double currentNumber = lexer.currentNumber();
				if (currentNumber != inputNumbers.at(step)) {
					BOOST_ERROR("for input " << inp
						<< ", check lexer.currentNumber() == inputNumbers.at(" << step << ") has failed ["
						<< currentNumber << " != " << inputNumbers.at(step) << "]");
				}
			});
		}
	}
}
