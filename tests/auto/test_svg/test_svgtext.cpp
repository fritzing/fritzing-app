#include <boost/test/unit_test.hpp>

#include "svg/svgtext.h"

#include <QApplication>
#include <QString>
#include <QDomDocument>
#include <QList>
#include <QImage>
#include <QMatrix>

BOOST_AUTO_TEST_CASE( svgtext_bounds )
{
	QString input =
R"x(
<svg
    xmlns="http://www.w3.org/2000/svg" xml:space="preserve"
    xmlns:xml="http://www.w3.org/XML/1998/namespace" enable-background="new 0 0 57.6 50.4" viewBox="0 0 57.6 50.4" width="0.8in" height="0.7in" version="1.1" id="svg2" gorn="0" y="0px" x="0px">
    <g>
        <g>svg.schematic.Optocoupler_TLP621_schematic.svg</g>
    </g>
    <g  id="defs51"/>
    <g  fill="#FFFFFF" stroke="#000000" width="43.195" height="36" stroke-width="0.9002" stroke-linecap="round" id="part_symbol" gorn="0.3" y="7.2" x="7.2"/>
    <g  id="schematic" gorn="0.4">
        <g transform="matrix(1 0 0 1 2.7751 12.7263)">
            <g >
                <g >
                    <g >
                        <text  fill="#787878" font-size="2" font-family="DroidSans" id="text23" gorn="0.4.13.0.0.0">1</text>
                    </g>
                </g>
            </g>
        </g>
        <g transform="matrix(1 0 0 1 2.7751 34.6501)">
            <g >
                <g >
                    <g >
                        <text  fill="#787878" font-size="2" font-family="DroidSans" id="text25" gorn="0.4.14.0.0.0">2</text>
                    </g>
                </g>
            </g>
        </g>
        <g  fill="none" y2="30.331" stroke="#000000" x2="38.625" stroke-width="0.7" stroke-linecap="round" x1="38.625" id="line27" gorn="0.4.15" y1="19.531"/>
        <g  fill="none" points="38.625,25.777    45.125,29.866 45.125,36 50.239,36  " stroke="#000000" stroke-width="0.7" stroke-linecap="round" id="connector1pin_4_" gorn="0.4.16"/>
        <g  fill="none" points="50.239,14.4    45.125,14.4 45.125,19.531 38.625,23.421  " stroke="#000000" stroke-width="0.7" stroke-linecap="round" id="connector1pin_2_" gorn="0.4.17"/>
        <g  fill="none" y2="36" stroke="#787878" x2="57.6" stroke-width="0.7" stroke-linecap="round" stroke-linejoin="round" x1="50.399" id="connector5pin" gorn="0.4.18" y1="36"/>
        <g  fill="none" y2="14.4" stroke="#787878" x2="57.6" stroke-width="0.7" stroke-linecap="round" stroke-linejoin="round" x1="50.399" id="connector3pin" gorn="0.4.19" y1="14.4"/>
        <g transform="matrix(1 0 0 1 53.1418 12.7263)">
            <g >
                <g >
                    <g >
                        <text  fill="#787878" font-size="2" font-family="DroidSans" id="text33" gorn="0.4.20.0.0.0">4</text>
                    </g>
                </g>
            </g>
        </g>
        <g transform="matrix(1 0 0 1 53.1418 34.6501)">
            <g >
                <g >
                    <g >
                        <text  fill="#787878" font-size="2" font-family="DroidSans" id="text35" gorn="0.4.21.0.0.0">3</text>
                    </g>
                </g>
            </g>
        </g>
        <g  fill="none" y2="23.76" stroke="#000000" x2="21.427" stroke-width="0.45" stroke-linecap="round" x1="23.767" id="line39" gorn="0.4.22" y1="26.101"/>
        <g  fill="none" y2="27.001" stroke="#000000" x2="21.066" stroke-width="0.45" stroke-linecap="round" x1="23.406" id="line41" gorn="0.4.23" y1="29.341"/>
        <g  d="M22.866,26.637l1.438-1.438l1.08,2.521L22.866,26.637z" id="path43" gorn="0.4.24"/>
        <g  d="M22.509,29.878l1.438-1.438l1.08,2.521L22.509,29.878z" id="path45" gorn="0.4.25"/>
        <g  d="M41.498,28.738l1.111-1.704l1.577,2.243L41.498,28.738z" id="path47" gorn="0.4.26"/>
    </g>
</svg>
)x";


    // A QApplication instance is necessary if fonts are used in the SVG
	int argc = 0;
	QApplication app(argc, nullptr);

    QDomDocument doc;
    QString errorStr;
    int errorLine;
    int errorColumn;
    if (!doc.setContent(input, &errorStr, &errorLine, &errorColumn)) {
        throw;
    }

    QDomElement root = doc.documentElement();
    QDomNodeList nodeList = root.elementsByTagName("text");
    QList<QDomElement> texts;
    for (int i = 0; i < nodeList.count(); i++) {
        texts.append(nodeList.at(i).toElement());
    }

    foreach (QDomElement text, texts) {
        text.setTagName("g");
    }

	QImage image(116, 102, QImage::Format_Mono);  // schematic text is so small it doesn't render unless bitmap is double-sized

    foreach (QDomElement text, texts) {
        // TextThing textThing;
        QRectF viewBox;
        QMatrix matrix;
		int minX=1;
		int minY=1;
		int maxX=0;
		int maxY=0;

		SvgText::renderText(image, text, minX, minY, maxX, maxY, matrix, viewBox);

		BOOST_REQUIRE_LE(minX,maxX);
		BOOST_REQUIRE_LE(minY,maxY);

    }

    foreach (QDomElement text, texts) {
        text.setTagName("text");
    }

}


