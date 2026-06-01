/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2026 Fritzing

Fritzing is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Fritzing is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************/

#include <QtTest/QtTest>
#include <QImage>
#include <QPainter>

#include "gerberpreview/gerberparser.h"
#include "gerberpreview/gerberdocument.h"
#include "gerberpreview/gerberrenderer.h"
#include "gerberpreview/excellonparser.h"

// Golden RS-274X: 10 x 10 mm box outline (4 D01 strokes) + one
// rectangular flash at the centre, drawn with two simple apertures.
// Format: leading-zero suppression, absolute, 4-int / 6-dec, mm.
static const char * kGoldenGerber =
	"%FSLAX46Y46*%\n"
	"%MOMM*%\n"
	"%ADD10C,0.5*%\n"
	"%ADD11R,1.0X2.0*%\n"
	"G01*\n"
	"D10*\n"
	"X0Y0D02*\n"
	"X10000000Y0D01*\n"
	"X10000000Y10000000D01*\n"
	"X0Y10000000D01*\n"
	"X0Y0D01*\n"
	"D11*\n"
	"X5000000Y5000000D03*\n"
	"M02*\n";

// Minimal Excellon drill file — two 0.8 mm holes, literal decimals
// so the test does not depend on leading/trailing-zero defaults.
static const char * kGoldenExcellon =
	"M48\n"
	"METRIC\n"
	"T1C0.8\n"
	"%\n"
	"T1\n"
	"X5.0Y5.0\n"
	"X10.0Y10.0\n"
	"M30\n";

class TestGerberPreview : public QObject {
	Q_OBJECT

private slots:
	void parsesApertures();
	void parsesCommands();
	void boundsCoverBox();
	void rendererProducesNonEmptyImage();
	void excellonHits();
};

void TestGerberPreview::parsesApertures() {
	GerberParser p;
	GerberDocument doc = p.parse(QString::fromLatin1(kGoldenGerber));
	// We declared D10 (circle) and D11 (rect). The parser must
	// surface both with their original codes and dimensions in mm.
	QVERIFY(doc.apertures.contains(10));
	QVERIFY(doc.apertures.contains(11));
	QCOMPARE(doc.apertures.value(10).kind, GerberAperture::Circle);
	QCOMPARE(doc.apertures.value(11).kind, GerberAperture::Rectangle);
	QVERIFY(qFuzzyCompare(doc.apertures.value(10).w, 0.5));
	QVERIFY(qFuzzyCompare(doc.apertures.value(11).w, 1.0));
	QVERIFY(qFuzzyCompare(doc.apertures.value(11).h, 2.0));
}

void TestGerberPreview::parsesCommands() {
	GerberParser p;
	GerberDocument doc = p.parse(QString::fromLatin1(kGoldenGerber));
	int strokes = 0, flashes = 0;
	for (const GerberCommand & c : doc.commands) {
		if (c.kind == GerberCommand::Stroke) ++strokes;
		else if (c.kind == GerberCommand::Flash) ++flashes;
	}
	// Four sides of the box = 4 strokes; one D03 = 1 flash.
	QCOMPARE(strokes, 4);
	QCOMPARE(flashes, 1);
}

void TestGerberPreview::boundsCoverBox() {
	GerberParser p;
	GerberDocument doc = p.parse(QString::fromLatin1(kGoldenGerber));
	// Box runs (0,0) -> (10,10) mm; slack from aperture half-width
	// is allowed but the inner extremes must be inside the bounds.
	QVERIFY(doc.bounds.left()   <= 0.0);
	QVERIFY(doc.bounds.right()  >= 10.0);
	QVERIFY(doc.bounds.top()    <= 0.0);
	QVERIFY(doc.bounds.bottom() >= 10.0);
}

void TestGerberPreview::rendererProducesNonEmptyImage() {
	GerberParser p;
	GerberDocument doc = p.parse(QString::fromLatin1(kGoldenGerber));

	QImage img(200, 200, QImage::Format_ARGB32_Premultiplied);
	img.fill(Qt::white);
	{
		QPainter painter(&img);
		painter.setRenderHint(QPainter::Antialiasing, true);
		// World->pixel: 15 px per mm, Y-flip so Gerber +Y goes up.
		QTransform t;
		t.translate(20, 180);
		t.scale(15.0, -15.0);
		painter.setTransform(t);

		GerberRenderer r;
		r.render(painter, doc, QColor(184, 115, 51, 200));
	}

	// At least one pixel must differ from the white background;
	// catches accidental no-op renderers or wrong-colour fills.
	bool nonWhite = false;
	for (int y = 0; y < img.height() && !nonWhite; ++y) {
		const QRgb * row = reinterpret_cast<const QRgb *>(img.scanLine(y));
		for (int x = 0; x < img.width(); ++x) {
			if (row[x] != 0xFFFFFFFFu) { nonWhite = true; break; }
		}
	}
	QVERIFY(nonWhite);
}

void TestGerberPreview::excellonHits() {
	ExcellonParser p;
	ExcellonParser::Result r = p.parse(QString::fromLatin1(kGoldenExcellon));
	QCOMPARE(r.hits.size(), 2);
	QVERIFY(qFuzzyCompare(r.hits.first().diameter, 0.8));
	// First hit at (5, 5) mm — independent of zero-suppression mode
	// because the input uses explicit decimals.
	QVERIFY(qFuzzyCompare(r.hits.first().pos.x(), 5.0));
	QVERIFY(qFuzzyCompare(r.hits.first().pos.y(), 5.0));
}

QTEST_MAIN(TestGerberPreview)
#include "test_gerberpreview.moc"
