/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2016 Fritzing

Fritzing is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, orF
(at your option) any later version.

Fritzing is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Fritzing.  If not, see <http://www.gnu.org/licenses/>.

********************************************************************

$Revision: 6941 $:
$Author: irascibl@gmail.com $:
$Date: 2013-03-26 15:03:18 +0100 (Di, 26. Mrz 2013) $

********************************************************************/

#include "groundplanegenerator.h"
#include "svgfilesplitter.h"
#include "../fsvgrenderer.h"
#include "../debugdialog.h"
#include "../version/version.h"
#include "../utils/folderutils.h"
#include "../utils/graphicsutils.h"
#include "../utils/textutils.h"
#include "../items/wire.h"
#include "../processeventblocker.h"
#include "../autoroute/drc.h"

#include <QBitArray>
#include <QPainter>
#include <QSvgRenderer>
#include <QDate>
#include <QTextStream>
#include <qmath.h>
#include <limits>
#include <QtConcurrentRun>

static const double BORDERINCHES = 0.04;

const QString GroundPlaneGenerator::KeepoutSettingName("GPG_Keepout");
const double GroundPlaneGenerator::KeepoutDefaultMils = 10;  

inline int OFFSET(int x, int y, QImage * image) { return (y * image->width()) + x; }

QString GroundPlaneGenerator::ConnectorName = "connector0pad";

//  !!!!!!!!!!!!!!!!!!!
//  !!!!!!!!!!!!!!!!!!!  IMPORTANT NOTE:  QRect::right() and QRect::bottom() are off by one--this is a known Qt problem 
//  !!!!!!!!!!!!!!!!!!!
//  !!!!!!!!!!!!!!!!!!!					  one workaround might be to switch to QRectF
//  !!!!!!!!!!!!!!!!!!!

GroundPlaneGenerator::GroundPlaneGenerator()
{
	m_strokeWidthIncrement = 0;
	m_minRiseSize = m_minRunSize = 1;
}

GroundPlaneGenerator::~GroundPlaneGenerator() {
}

bool GroundPlaneGenerator::getBoardRects(const QByteArray & boardByteArray, QGraphicsItem * board, double res, double keepoutSpace, QList<QRect> & rects)
{

	QRectF br = board->sceneBoundingRect();
	double bWidth = res * br.width() / GraphicsUtils::SVGDPI;
	double bHeight = res * br.height() / GraphicsUtils::SVGDPI;
	QImage image(bWidth, bHeight, QImage::Format_Mono);  
	image.setDotsPerMeterX(res * GraphicsUtils::InchesPerMeter);
	image.setDotsPerMeterY(res * GraphicsUtils::InchesPerMeter);
	image.fill(0xffffffff);

	QSvgRenderer renderer(boardByteArray);
	QPainter painter;
	painter.begin(&image);
	painter.setRenderHint(QPainter::Antialiasing, false);
	renderer.render(&painter);
	painter.end();

#ifndef QT_NO_DEBUG
    image.save(FolderUtils::getTopLevelUserDataStorePath() + "/getBoardRects.png");
#endif

	QColor keepaway(255,255,255);

	// now add keepout area to the border
	QImage image2 = image.copy();
	painter.begin(&image2);
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.fillRect(0, 0, image2.width(), keepoutSpace, keepaway);
	painter.fillRect(0, image2.height() - keepoutSpace, image2.width(), keepoutSpace, keepaway);
	painter.fillRect(0, 0, keepoutSpace, image2.height(), keepaway);
	painter.fillRect(image2.width() - keepoutSpace, 0, keepoutSpace, image2.height(), keepaway);

	for (int y = 0; y < image.height(); y++) {
		for (int x = 0; x < image.width(); x++) {
			QRgb current = image.pixel(x, y);
			if (current != 0xffffffff) {			
				continue;
			}

			painter.fillRect(x - keepoutSpace, y - keepoutSpace, keepoutSpace + keepoutSpace, keepoutSpace + keepoutSpace, keepaway);
		}
	}
	painter.end();

#ifndef QT_NO_DEBUG
    image2.save(FolderUtils::getTopLevelUserDataStorePath() + "/getBoardRects2.png");
#endif

	scanLines(image2, bWidth, bHeight, rects);

	// combine parallel equal-sized rects
	int ix = 0;
	while (ix < rects.count()) {
		QRect r = rects.at(ix++);
		for (int j = ix; j < rects.count(); j++) {
			QRect s = rects.at(j);
			if (s.bottom() == r.bottom()) {
				// on same row; keep going
				continue;
			}

			if (s.top() > r.bottom() + 1) {
				// skipped row, can't join
				break;
			}

			if (s.left() == r.left() && s.right() == r.right()) {
				// join these
				r.setBottom(s.bottom());
				rects.removeAt(j);
				ix--;
				rects.replace(ix, r);
				break;
			}
		}
	}

	return true;
}

bool GroundPlaneGenerator::generateGroundPlaneUnit(const QString & boardSvg, QSizeF boardImageSize, const QString & svg, QSizeF copperImageSize, 
												   QStringList & exceptions, QGraphicsItem * board, double res, const QString & color,
												   QPointF whereToStart, double keepoutMils) 
{
    GPGParams params;
    params.boardSvg = boardSvg;
    params.boardImageSize = boardImageSize;
    params.svg = svg;
    params.copperImageSize =  copperImageSize;
	params.exceptions = exceptions;
    params.board = board;
    params.res = res;
    params.color = color;
    params.keepoutMils = keepoutMils;

    double bWidth, bHeight;
    QList<QRectF> rects;
	QImage * image = generateGroundPlaneAux(params, bWidth, bHeight, rects);
	if (image == NULL) return false;

	QRectF bsbr = board->sceneBoundingRect();

	QPoint s(qRound(res * (whereToStart.x() - bsbr.topLeft().x()) / GraphicsUtils::SVGDPI),
			qRound(res * (whereToStart.y() - bsbr.topLeft().y()) / GraphicsUtils::SVGDPI));

	QBitArray redMarker(image->height() * image->width(), false);

	QRgb pixel = image->pixel(s);
	//DebugDialog::debug(QString("unit %1").arg(pixel, 0, 16));
	if (pixel != 0xffffffff) {
		// starting off in bad territory
		delete image;
		return false;
	}

	// step 1 flood fill white to "red" (keep max locations)

	int minY = image->height();
	int maxY = 0;
	int minX = image->width();
	int maxX = 0;
	QList<QPoint> stack;
	stack << s;
	while (stack.count() > 0) {
		QPoint p = stack.takeFirst();

		if (p.x() < 0) continue;
		if (p.y() < 0) continue;
		if (p.x() >= image->width()) continue;
		if (p.y() >= image->height()) continue;
		if (redMarker.testBit(OFFSET(p.x(), p.y(), image))) continue;			// already been here

		QRgb pixel = image->pixel(p);
		if (pixel != 0xffffffff) continue;			// black; bail

		redMarker.setBit(OFFSET(p.x(), p.y(), image), true);
		if (p.x() > maxX) maxX = p.x();
		if (p.x() < minX) minX = p.x();
		if (p.y() > maxY) maxY = p.y();
		if (p.y() < minY) minY = p.y();

		stack.append(QPoint(p.x() - 1, p.y()));
		stack.append(QPoint(p.x() + 1, p.y()));
		stack.append(QPoint(p.x(), p.y() - 1));
		stack.append(QPoint(p.x(), p.y() + 1));
	}

	//image->save("testPoly1.png");

	// step 2 replace white with black
	image->fill(0);

	// step 3 replace "red" with white
	for (int y = 0; y < image->height(); y++) {
		for (int x = 0; x < image->width(); x++) {
			if (redMarker.testBit(OFFSET(x, y, image))) {
				image->setPixel(x, y, 1);
			}
		}
	}

#ifndef QT_NO_DEBUG
    image->save(FolderUtils::getTopLevelUserDataStorePath() + "/testGroundPlaneUnit3.png");
#endif

	scanImage(*image, bWidth, bHeight, GraphicsUtils::StandardFritzingDPI / res, res, color, true, true, QSizeF(.05, .05), 1 / GraphicsUtils::SVGDPI, QPointF(0,0));
	delete image;
	return true;
}

bool GroundPlaneGenerator::generateGroundPlane(const QString & boardSvg, QSizeF boardImageSize, const QString & svg, QSizeF copperImageSize, 
												QStringList & exceptions, QGraphicsItem * board, double res, const QString & color, double keepoutMils) 
{
    GPGParams params;
    params.boardSvg = boardSvg;
    params.keepoutMils = keepoutMils;
    params.boardImageSize = boardImageSize;
    params.svg = svg;
    params.copperImageSize =  copperImageSize;
	params.exceptions = exceptions;
    params.board = board;
    params.res = res;
    params.color = color;
    QFuture<bool> future = QtConcurrent::run(this, &GroundPlaneGenerator::generateGroundPlaneFn, params);
    while (!future.isFinished()) {
        ProcessEventBlocker::processEvents(200);
    }
    return future.result();
}

bool GroundPlaneGenerator::generateGroundPlaneFn(GPGParams & params) 
{

	double bWidth, bHeight;
    QList<QRectF> rects;
	QImage * image = generateGroundPlaneAux(params, bWidth, bHeight, rects);
	if (image == NULL) return false;

    double pixelFactor = GraphicsUtils::StandardFritzingDPI / params.res;
	scanImage(*image, bWidth, bHeight, pixelFactor, params.res, params.color, true, true, QSizeF(.05, .05), 1 / GraphicsUtils::SVGDPI, QPointF(0,0));
    if (rects.count() > 0) {
        foreach (QRectF r, rects) {
            // add the rects separately as tiny SVGs which don't get clipped (since they are connected)
            QList<QPolygon> polygons;
            QPolygon polygon;
            polygon << QPoint(r.left() * pixelFactor, r.top() * pixelFactor) 
                    << QPoint(r.right() * pixelFactor, r.top() * pixelFactor)  
                    << QPoint(r.right() * pixelFactor, r.bottom() * pixelFactor) 
                    << QPoint(r.left() * pixelFactor, r.bottom() * pixelFactor);
            polygons.append(polygon);
            makePolySvg(polygons, params.res, bWidth, bHeight, pixelFactor, params.color, false, true, QSizeF(0, 0), 0, QPointF(0, 0));
        }
    }

	delete image;
	return true;
}

QImage * GroundPlaneGenerator::generateGroundPlaneAux(GPGParams & params, double & bWidth, double & bHeight, QList<QRectF> & rects) 
{
	QByteArray boardByteArray;
    QString tempColor("#ffffff");
    if (!SvgFileSplitter::changeColors(params.boardSvg, tempColor, params.exceptions, boardByteArray)) {
		return NULL;
	}

	
	//QFile file0("testGroundFillBoard.svg");
	//file0.open(QIODevice::WriteOnly);
	//QTextStream out0(&file0);
	//out0 << boardByteArray;
	//file0.close();
	

    /*
    QByteArray copperByteArray;
    if (!SvgFileSplitter::changeStrokeWidth(params.svg, m_strokeWidthIncrement, false, true, copperByteArray)) {
		return NULL;
	}
    */

	QString errorStr;
	int errorLine;
	int errorColumn;
    QDomDocument doc;
    doc.setContent(params.svg, &errorStr, &errorLine, &errorColumn);
    QDomElement root = doc.documentElement();
    SvgFileSplitter::forceStrokeWidth(root, 2 * params.keepoutMils, "#000000", true, true);
    QByteArray copperByteArray = doc.toByteArray(0);
	
	//QFile file1("testGroundFillCopper.svg");
	//file1.open(QIODevice::WriteOnly);
	//QTextStream out1(&file1);
	//out1 << copperByteArray;
	//file1.close();
	

	double svgWidth = params.res * qMax(params.boardImageSize.width(), params.copperImageSize.width()) / GraphicsUtils::SVGDPI;
	double svgHeight = params.res * qMax(params.boardImageSize.height(), params.copperImageSize.height()) / GraphicsUtils::SVGDPI;

	QRectF br =  params.board->sceneBoundingRect();
	bWidth = params.res * br.width() / GraphicsUtils::SVGDPI;
	bHeight = params.res * br.height() / GraphicsUtils::SVGDPI;
	QImage * image = new QImage(qMax(svgWidth, bWidth), qMax(svgHeight, bHeight), QImage::Format_Mono); //
	image->setDotsPerMeterX(params.res * GraphicsUtils::InchesPerMeter);
	image->setDotsPerMeterY(params.res * GraphicsUtils::InchesPerMeter);
	image->fill(0x0);

	QSvgRenderer renderer(boardByteArray);
	QPainter painter;
	painter.begin(image);
	painter.setRenderHint(QPainter::Antialiasing, false);
	QRectF boardBounds(0, 0, params.res * params.boardImageSize.width() / GraphicsUtils::SVGDPI, params.res * params.boardImageSize.height() / GraphicsUtils::SVGDPI); 
	DebugDialog::debug("boardbounds", boardBounds);
	renderer.render(&painter, boardBounds);
	painter.end();

#ifndef QT_NO_DEBUG
    image->save(FolderUtils::getTopLevelUserDataStorePath() + "/testGroundFillBoard.png");
#endif

    DRC::extendBorder(BORDERINCHES * params.res, image);
    GraphicsUtils::drawBorder(image, BORDERINCHES * params.res);

    QImage boardImage = image->copy();

    /*
	for (double m = 0; m < BORDERINCHES; m += (1.0 / params.res)) {   // 1 mm
		QList<QPoint> points;
		collectBorderPoints(*image, points);

#ifndef QT_NO_DEBUG

		// for debugging
		//double pixelFactor = GraphicsUtils::StandardFritzingDPI / res;
		//QPolygon polygon;
		//foreach(QPoint p, points) {
		//	polygon.append(QPoint(p.x() * pixelFactor, p.y() * pixelFactor));
		//}

		//QList<QPolygon> polygons;
		//polygons.append(polygon);
		//QPointF offset;
		//this
		//QString pSvg = makePolySvg(polygons, res, bWidth, bHeight, pixelFactor, "#ffffff", false,  NULL, QSizeF(0,0), 0, QPointF(0, 0));
		
#endif

		foreach (QPoint p, points) image->setPixel(p, 0);
	}
    */

#ifndef QT_NO_DEBUG
    image->save(FolderUtils::getTopLevelUserDataStorePath() + "/testGroundFillBoardBorder.png");
#endif
	
	QSvgRenderer renderer2(copperByteArray);
	painter.begin(image);
	painter.setRenderHint(QPainter::Antialiasing, false);
	QRectF bounds(0, 0, params.res * params.copperImageSize.width() / GraphicsUtils::SVGDPI, params.res * params.copperImageSize.height() / GraphicsUtils::SVGDPI);
	DebugDialog::debug("copperbounds", bounds);
	renderer2.render(&painter, bounds);
	painter.end();

#ifndef QT_NO_DEBUG
    image->save(FolderUtils::getTopLevelUserDataStorePath() + "/testGroundFillCopper.png");
#endif

	emit postImageSignal(this, image, &boardImage, params.board, &rects);	

	return image;
}

void GroundPlaneGenerator::scanImage(QImage & image, double bWidth, double bHeight, double pixelFactor, double res, 
									 const QString & colorString, bool makeConnectorFlag, 
									 bool makeOffset, QSizeF minAreaInches, double minDimensionInches, QPointF polygonOffset)  
{
	QList<QRect> rects;
	scanLines(image, bWidth, bHeight, rects);
	QList< QList<int> * > pieces;
	splitScanLines(rects, pieces);
	foreach (QList<int> * piece, pieces) {
		QList<QPolygon> polygons;
		QList<QRect> newRects;
		foreach (int i, *piece) {
			QRect r = rects.at(i);
			newRects.append(QRect(r.x() * pixelFactor, r.y() * pixelFactor, (r.width() * pixelFactor) + 1, pixelFactor + 1));    // + 1 is for off-by-one converting rects to polys
		}

		// note: there is always one
		joinScanLines(newRects, polygons);
        makePolySvg(polygons, res, bWidth, bHeight, pixelFactor, colorString, makeConnectorFlag, makeOffset, minAreaInches, minDimensionInches, polygonOffset);
	}

	/*
	QString newSvg = QString("<svg xmlns='http://www.w3.org/2000/svg' width='%1in' height='%2in' viewBox='0 0 %3 %4' >\n")
		.arg(bWidth / res)
		.arg(bHeight / res)
		.arg(bWidth * MILS)
		.arg(bHeight * MILS);
	newSvg += "<g id='groundplane'>\n";

	// ?split each line into two lines (l1, l2) and add a terminal point at the left of l1 and the right of l2?

	ix = 0;
	foreach (QRectF r, rects) {
		newSvg += QString("<rect x='%1' y='%2' width='%3' height='%4' id='connector%5pad' fill='%6'  />\n")
			.arg(r.left() * MILS)
			.arg(r.top() * MILS)
			.arg(r.width() * MILS)
			.arg(MILS)
			.arg(ix++).
			.arg(ViewLayer::Copper0Color);
	}
	newSvg += "</g>\n</svg>\n";
	*/

}

void GroundPlaneGenerator::scanLines(QImage & image, int bWidth, int bHeight, QList<QRect> & rects)
{
	if (m_minRiseSize > 1) {
		for (int x = 0; x < bWidth; x++) {
			bool inWhite = false;
			int whiteStart = 0;
			for (int y = 0; y < bHeight; y++) {
				QRgb current = image.pixel(x, y);
				if (inWhite) {
					if (current == 0xffffffff) {			// qBlue(current) == 0xff    gray > 128
						// another white pixel, keep moving
						continue;
					}

					// got black: close up this segment;
					inWhite = false;
					if (y - whiteStart < m_minRiseSize) {
						for (int j = whiteStart; j <= y; j++) {
							image.setPixel(x, j, 0);
						}
						continue;
					}

				}
				else {
					if (current != 0xffffffff) {		// qBlue(current) != 0xff				
						// another black pixel, keep moving
						continue;
					}

					inWhite = true;
					whiteStart = y;
				}
			}
			if (inWhite) {
				// close up the last segment
				if (bHeight - whiteStart < m_minRiseSize) {
					for (int j = whiteStart; j <= bHeight; j++) {
						image.setPixel(x, j, 0);
					}			
				}
			}
		}
	}

	// threshold should be between 0 and 255 exclusive; smaller will include more of the svg
	for (int y = 0; y < bHeight; y++) {
		bool inWhite = false;
		int whiteStart = 0;
		for (int x = 0; x < bWidth; x++) {
			QRgb current = image.pixel(x, y);
			//if (current != 0xff000000 && current != 0xffffffff) {
				//DebugDialog::debug(QString("current %1").arg(current,0,16));
			//}
			//DebugDialog::debug(QString("current %1 %2").arg(current,0,16).arg(gray, 0, 16));
			if (inWhite) {
				if (current == 0xffffffff) {			// qBlue(current) == 0xff    gray > 128
					// another white pixel, keep moving
					continue;
				}

				// got black: close up this segment;
				inWhite = false;
				if (x - whiteStart < m_minRunSize) {
					// not a big enough section
					continue;
				}

				rects.append(QRect(whiteStart, y, x - whiteStart, 1));
			}
			else {
				if (current != 0xffffffff) {		// qBlue(current) != 0xff				
					// another black pixel, keep moving
					continue;
				}

				inWhite = true;
				whiteStart = x;
			}
		}
		if (inWhite) {
			// close up the last segment
			if (bWidth - whiteStart >= m_minRunSize) {
				rects.append(QRect(whiteStart, y, bWidth - whiteStart, 1));
			}
		}
	}
}

void GroundPlaneGenerator::splitScanLines(QList<QRect> & rects, QList< QList<int> * > & pieces) 
{
	// combines vertically adjacent scanlines into "pieces"
	int ix = 0;
	int prevFirst = -1;
	int prevLast = -1;
	while (ix < rects.count()) {
		int first = ix;
		QRectF firstR = rects.at(ix);
		while (++ix < rects.count()) {
			QRectF nextR = rects.at(ix);
			if (nextR.y() != firstR.y()) {
				break;
			}
		}
		int last = ix - 1;  // this was a lookahead so step back one
		if (prevFirst >= 0) {
			for (int i = first; i <= last; i++) {
				QRectF candidate = rects.at(i);
				int gotCount = 0;
				for (int j = prevFirst; j <= prevLast; j++) {
					QRectF prev = rects.at(j);
					if (prev.y() + 1 != candidate.y()) {
						// skipped a line; no intersection possible
						break;
					}

					if ((prev.x() + prev.width() <= candidate.x()) || (candidate.x() + candidate.width() <= prev.x())) {
						// candidate and prev didn't intersect
						continue;
					}

					if (++gotCount > 1) {
						QList<int> * piecei = NULL;
						QList<int> * piecej = NULL;
						foreach (QList<int> * piece, pieces) {
							if (piece->contains(j)) {
								piecej = piece;
								break;
							}
						}
						foreach (QList<int> * piece, pieces) {
							if (piece->contains(i)) {
								piecei = piece;
								break;
							}
						}
						if (piecei != NULL && piecej != NULL) {
							if (piecei != piecej) {
								foreach (int b, *piecej) {
									piecei->append(b);
								}
								piecej->clear();
								pieces.removeOne(piecej);
								delete piecej;
							}
							piecei->append(i);
						}
						else {
							DebugDialog::debug("we are really screwed here, what should we do about it?");
						}
					}
					else {
						// put the candidate (i) in j's piece
						foreach (QList<int> * piece, pieces) {
							if (piece->contains(j)) {
								piece->append(i);
								break;
							}
						}
					}
				}

				if (gotCount == 0) {
					// candidate is an orphan line at this point
					QList<int> * piece = new QList<int>;
					piece->append(i);
					pieces.append(piece);
				}

			}
		}
		else {
			for (int i = first; i <= last; i++) {
				QList<int> * piece = new QList<int>;
				piece->append(i);
				pieces.append(piece);
			}
		}

		prevFirst = first;
		prevLast = last;
	}

	foreach (QList<int> * piece, pieces) {
		qSort(*piece);
	}
}

void GroundPlaneGenerator::joinScanLines(QList<QRect> & rects, QList<QPolygon> & polygons) {
	QList< QList<int> * > pieces;
	int ix = 0;
	int prevFirst = -1;
	int prevLast = -1;
	while (ix < rects.count()) {
		int first = ix;
		QRectF firstR = rects.at(ix);
		while (++ix < rects.count()) {
			QRectF nextR = rects.at(ix);
			if (nextR.y() != firstR.y()) {
				break;
			}
		}
		int last = ix - 1;  
		if (prevFirst >= 0) {
			QVector<int> holdPrevs(last - first + 1);
			QVector<int> gotCounts(last - first + 1);
			for (int i = first; i <= last; i++) {
				int index = i - first;
				holdPrevs[index] = 0;
				gotCounts[index] = 0;
				QRectF candidate = rects.at(i);
				for (int j = prevFirst; j <= prevLast; j++) {
					QRectF prev = rects.at(j);

					if ((prev.x() + prev.width() <= candidate.x()) || (candidate.x() + candidate.width() <= prev.x())) {
						// candidate and prev didn't intersect
						continue;
					}

					holdPrevs[index] = j;
					gotCounts[index]++;
				}
				if (gotCounts[index] > 1) {
					holdPrevs[index] = -1;			// clear this to allow one of the others in this scanline to capture a previous
				}
			}
			for (int i = first; i <= last; i++) {
				int index = i - first;

				bool gotOne = false;
				if (gotCounts[index] == 1) {
					bool unique = true;
					for (int j = first; j <= last; j++) {
						if (j - first == index) continue;			// don't compare against yourself

						if (holdPrevs[index] == holdPrevs[j - first]) {
							unique = false;
							break;
						}
					}

					if (unique) {
						// add this to the previous chunk
						gotOne = true;
						foreach (QList<int> * piece, pieces) {
							if (piece->contains(holdPrevs[index])) {
								piece->append(i);
								break;
							
							}
						}
					}
				}
				if (!gotOne) {
					// start a new chunk
					holdPrevs[index] = -1;						// allow others to capture the prev
					QList<int> * piece = new QList<int>;
					piece->append(i);
					pieces.append(piece);
				}
			}
		}
		else {
			for (int i = first; i <= last; i++) {
				QList<int> * piece = new QList<int>;
				piece->append(i);
				pieces.append(piece);
			}
		}

		prevFirst = first;
		prevLast = last;
	}

	foreach (QList<int> * piece, pieces) {
		//QPolygon poly(rects.at(piece->at(0)), true);
		//for (int i = 1; i < piece->length(); i++) {
			//QPolygon temp(rects.at(piece->at(i)), true);
			//poly = poly.united(temp);
		//}

		// no need to close polygon; SVG automatically closes path
		
		QPolygon poly;

		// left side
		for (int i = 0; i < piece->length(); i++) {
			QRect r = rects.at(piece->at(i));
			if ((poly.count() > 0) && (poly.last().x() == r.left())) {
				poly.pop_back();
			}
			else {
				poly.append(QPoint(r.left(), r.top()));
			}
			poly.append(QPoint(r.left(), r.bottom()));
		}
		// right side
		for (int i = piece->length() - 1; i >= 0; i--) {
			QRect r = rects.at(piece->at(i));
			if ((poly.count() > 0) && (poly.last().x() == r.right())) {
				poly.pop_back();
			}
			else {
				poly.append(QPoint(r.right(), r.bottom()));
			}
			poly.append(QPoint(r.right(), r.top()));
		}

		

		polygons.append(poly);
		delete piece;
	}
}

void GroundPlaneGenerator::makePolySvg(QList<QPolygon> & polygons, double res, double bWidth, double bHeight, double pixelFactor, 
										const QString & colorString, bool makeConnectorFlag, bool makeOffset, 
										QSizeF minAreaInches, double minDimensionInches, QPointF polygonOffset) 
{
	QPointF offset;
	QString pSvg = makePolySvg(polygons, res, bWidth, bHeight, pixelFactor, colorString, makeConnectorFlag, makeOffset ? &offset : NULL, minAreaInches, minDimensionInches, polygonOffset);
	if (pSvg.isEmpty()) return;

	m_newSVGs.append(pSvg);
	if (makeOffset) {
		offset *= GraphicsUtils::SVGDPI;
		m_newOffsets.append(offset);			// offset now in pixels
	}

	/*
	QFile file4("testPoly.svg");
	file4.open(QIODevice::WriteOnly);
	QTextStream out4(&file4);
	out4 << pSvg;
	file4.close();
	*/
}

QString GroundPlaneGenerator::makePolySvg(QList<QPolygon> & polygons, double res, double bWidth, double bHeight, double pixelFactor, 
										const QString & colorString, bool makeConnectorFlag, QPointF * offset, 
										QSizeF minAreaInches, double minDimensionInches, QPointF polygonOffset) 
{
	int minX = 0;
	int minY = 0;
	if (offset != NULL) {
		minY = std::numeric_limits<int>::max();
		int maxY = std::numeric_limits<int>::min();
		minX = minY;
		int maxX = maxY;

		foreach (QPolygon polygon, polygons) {
			foreach (QPoint p, polygon) {
				if (p.x() > maxX) maxX = p.x();
				if (p.x() < minX) minX = p.x();
				if (p.y() > maxY) maxY = p.y();
				if (p.y() < minY) minY = p.y();
			}
		}

		bWidth = (maxX - minX) / pixelFactor;
		bHeight = (maxY - minY) / pixelFactor;
		offset->setX(minX / (res * pixelFactor));		// inches
		offset->setY(minY / (res * pixelFactor));		// inches
	}

	if ((bWidth / res < minAreaInches.width()) && (bHeight / res < minAreaInches.height())) {
		return "";
	}
	if ((bWidth / res < minDimensionInches) || (bHeight / res < minDimensionInches)) {
		return "";
	}


	QString pSvg = QString("<svg xmlns='http://www.w3.org/2000/svg' width='%1in' height='%2in' viewBox='0 0 %3 %4' >\n")
		.arg(bWidth / res)
		.arg(bHeight / res)
		.arg(bWidth * pixelFactor)
		.arg(bHeight * pixelFactor);
	QString transform;
	if (polygonOffset.x() != 0 || polygonOffset.y() != 0) {
		transform = QString("transform='translate(%1, %2)'").arg(polygonOffset.x()).arg(polygonOffset.y());
	}
	pSvg += QString("<g id='%1' %2>\n").arg(m_layerName).arg(transform);
	if (makeConnectorFlag) {
		makeConnector(polygons, res, pixelFactor, colorString, minX, minY, pSvg);
	}
	else {
		foreach (QPolygon poly, polygons) {
			pSvg += makeOnePoly(poly, colorString, "", minX, minY);
		}
	}

	pSvg += "</g>\n</svg>\n";

	return pSvg;
}

void GroundPlaneGenerator::makeConnector(QList<QPolygon> & polygons, double res, double pixelFactor, const QString & colorString, int minX, int minY, QString & pSvg)
{
	//	see whether the standard circular connector will fit somewhere inside a polygon:
	//	http://stackoverflow.com/questions/4279478/maximum-circle-inside-a-non-convex-polygon
	//	or maybe this is useful, e.g. treating the circle as a square:  
	//	http://stackoverflow.com/questions/4833802/check-if-polygon-is-inside-a-polygon

	//	code presently uses a version of the Poles of Inaccessibility algorithm:

	static const double standardConnectorWidth = .075;		 // inches
	double targetDiameter = res * pixelFactor * standardConnectorWidth;
	double targetDiameterAnd = targetDiameter * 1.25;
	double targetRadius = targetDiameter / 2;
	double targetRadiusAnd = targetDiameterAnd / 2;
	double targetRadiusAndSquared = targetRadiusAnd * targetRadiusAnd;
	foreach (QPolygon poly, polygons) {
		QRect boundingRect = poly.boundingRect(); 
		if (boundingRect.width() < targetDiameterAnd) continue;
		if (boundingRect.height() < targetDiameterAnd) continue;

		QList<QLineF> polyLines;
		int count = poly.count();
		for (int i = 0; i < count; i++) {
			QLineF lp(poly[i], poly[(i + 1) % count]);
			polyLines.append(lp);
		}

		int xDivisor = qRound(boundingRect.width() / targetRadius);
		int yDivisor = qRound(boundingRect.height() / targetRadius);

		double dx = (boundingRect.width() - targetDiameterAnd) / xDivisor;
		double dy = (boundingRect.height() - targetDiameterAnd) / yDivisor;
		double x;
		double y = boundingRect.top() + targetRadiusAnd - dy;
		for (int iy = 0; iy <= yDivisor; iy++) {
			y += dy;
			x = boundingRect.left() + targetRadiusAnd - dx;
			for (int ix = 0; ix <= xDivisor; ix++) {
				x += dx;
				if (!poly.containsPoint(QPoint(qRound(x), qRound(y)), Qt::OddEvenFill)) continue;

				bool gotOne = true;
				foreach (QLineF line, polyLines) {
					double distance, dx, dy;
					bool atEndpoint;
					GraphicsUtils::distanceFromLine(x, y, line.p1().x(), line.p1().y(), line.p2().x(), line.p2().y(), 
													dx, dy, distance, atEndpoint);
					if (distance <= targetRadiusAndSquared) {
						gotOne = false;
						break;
					}
				}

				if (!gotOne) continue;

				foreach (QPolygon poly, polygons) {
					pSvg += makeOnePoly(poly, colorString, "", minX, minY);
				}

				pSvg += QString("<g id='%1'><circle cx='%2' cy='%3' r='%4' fill='%5' stroke='none' stroke-width='0' /></g>\n")
					.arg(ConnectorName)
					.arg(x - minX)
					.arg(y - minY)
					.arg(targetRadius)
					.arg(colorString);


				return;
			}
		}
	}

	// couldn't find anything big enough above, so
	// try to find a poly with an area that's big enough to click, but not so big as to get in the way
	int useIndex = -1;
	QList<double> areas;
	double divisor = res * pixelFactor * res * pixelFactor;
	foreach (QPolygon poly, polygons) {
		areas.append(calcArea(poly) / divisor);
	}
		
	for (int i = 0; i < areas.count(); i++) {
		if (areas.at(i) > 0.1 && areas.at(i) < 0.25) {
			useIndex = i;
			break;
		}
	}
	if (useIndex < 0) {
		for (int i = 0; i < areas.count(); i++) {
			if (areas.at(i) > 0.1) {
				useIndex = i;
				break;
			}
		}
	}
	if (useIndex < 0) {
		pSvg += QString("<g id='%1'>\n").arg(ConnectorName);
		foreach (QPolygon poly, polygons) {
			pSvg += makeOnePoly(poly, colorString, "", minX, minY);
		}
		pSvg += "</g>";
	}
	else {
		int ix = 0;
		for (int i = 0; i < polygons.count(); i++) {
			if (i == useIndex) {
				// has to appear inside a g element
				pSvg += QString("<g id='%1'>\n").arg(ConnectorName);
				pSvg += makeOnePoly(polygons.at(i), colorString, "", minX, minY);
				pSvg += "</g>";
			}
			else {
				pSvg += makeOnePoly(polygons.at(i), colorString, FSvgRenderer::NonConnectorName + QString::number(ix++), minX, minY);
			}
		}
	}
}

double GroundPlaneGenerator::calcArea(QPolygon & poly) {
	double total = 0;
	for (int ix = 0; ix < poly.count(); ix++) {
		QPoint p0 = poly.at(ix);
		QPoint p1 = poly.at((ix + 1) % poly.count());
		total += (p0.x() * p1.y() - p1.x() * p0.y());
	}
	return qAbs(total / 2.0);
}

QString GroundPlaneGenerator::makeOnePoly(const QPolygon & poly, const QString & colorString, const QString & id, int minX, int minY) {
	QString idString;
	if (!id.isEmpty()) {
		idString = QString("id='%1'").arg(id);
	}
	QString polyString = QString("<polygon fill='%1' stroke='none' stroke-width='0' %2 points='\n").arg(colorString).arg(idString);
	int space = 0;
	foreach (QPoint p, poly) {
		polyString += QString("%1,%2 %3").arg(p.x() - minX).arg(p.y() - minY).arg((++space % 8 == 0) ?  "\n" : "");
	}
	polyString += "'/>\n";
	return polyString;
}

const QStringList & GroundPlaneGenerator::newSVGs() {
	return m_newSVGs;
}

const QList<QPointF> & GroundPlaneGenerator::newOffsets() {
	return m_newOffsets;
}

void removeRedundant(QList<QPoint> & points)
{
	QPoint current = points.last();
	int ix = points.count() - 2;
	int soFar = 1;
	while (ix > 0) {
		if (points.at(ix).x() != current.x()) {
			current = points.at(ix--);
			soFar = 1;
			continue;
		}

		if (++soFar > 2) {
			points.removeAt(ix + 1);
		}
		ix--;
	}
}

bool GroundPlaneGenerator::collectBorderPoints(QImage & image, QList<QPoint> & points)
{
	// background is black

    int currentX = 0, currentY = 0;
	bool gotSomething = false;

	for (int y = 0; y < image.height(); y++) {
		for (int x = 0; x < image.width(); x++) {
			QRgb current = image.pixel(x, y);
			if (current != 0xffffffff) {				
				// another black pixel, keep moving
				continue;
			}

			currentX = x;
			currentY = y;
			//DebugDialog::debug(QString("first point %1 %2").arg(currentX).arg(currentY));
			points.append(QPoint(currentX, currentY));
			gotSomething = true;
			break;
		}
		if (gotSomething) break;
	}

	if (!gotSomething) {
		DebugDialog::debug("first border point not found");
		return false;
	}

    bool done = false;
    long maxPoints = image.height() * image.width() / 2;
	for (long inc = 0; inc < maxPoints; inc++) {
        if (try8(currentX, currentY, image, points)) ;
		else {            
            QPoint p = points.first();
            if (qAbs(p.x() - currentX) < 4 && qAbs(p.y() - currentY) < 4) {
                // we're near the beginning again
                done = true;
                break;
            }

            bool keepGoing = false;
            for (int ix = points.count() - 2; ix >= 0; ix--) {
                QPoint p = points.at(ix);
                if (try8(p.x(), p.y(), image, points)) {
                    keepGoing = true; 
                    break;
                }
            }

            if (!keepGoing) break;
        }

		QPoint p = points.last();
		currentX = p.x();
		currentY = p.y();
        //DebugDialog::debug(QString("next point %1 %2").arg(currentX).arg(currentY));
        //if (inc % 100 == 0) {
            //DebugDialog::debug("\n");
        //}
	}
    return done;
}

void GroundPlaneGenerator::scanOutline(QImage & image, double bWidth, double bHeight, double pixelFactor, double res, 
									 const QString & colorString, bool makeConnectorFlag, 
									 bool makeOffset, QSizeF minAreaInches, double minDimensionInches)  
{
	QList<QPoint> points;

	bool result = collectBorderPoints(image, points);
	if (points.count() == 0 || !result) {
		DebugDialog::debug("no border points");
		return;
	}

	removeRedundant(points);

	QPolygon polygon;
	foreach(QPoint p, points) {
		polygon.append(QPoint(p.x() * pixelFactor, p.y() * pixelFactor));
	}

	QList<QPolygon> polygons;
	polygons.append(polygon);
    makePolySvg(polygons, res, bWidth, bHeight, pixelFactor, colorString, makeConnectorFlag, makeOffset, minAreaInches, minDimensionInches, QPointF(0,0));

	/*
	QFile file4("testPoly.svg");
	file4.open(QIODevice::WriteOnly);
	QTextStream out4(&file4);
	out4 << pSvg;
	file4.close();
	*/
}


bool GroundPlaneGenerator::try8(int x, int y, QImage & image, QList<QPoint> & points) {
    if (tryNextPoint(x, y + 1, image, points)) return true;
	else if (tryNextPoint(x + 1, y, image, points)) return true;
	else if (tryNextPoint(x, y - 1, image, points)) return true;
	else if (tryNextPoint(x - 1, y, image, points)) return true;
	else if (tryNextPoint(x + 1, y + 1, image, points)) return true;
	else if (tryNextPoint(x - 1, y + 1, image, points)) return true;
	else if (tryNextPoint(x + 1, y - 1, image, points)) return true;
	else if (tryNextPoint(x - 1, y - 1, image, points)) return true;
    return false;
}

bool GroundPlaneGenerator::tryNextPoint(int x, int y, QImage & image, QList<QPoint> & points)
{
	if (x < 0) return false;
	if (y < 0) return false;
	if (x >= image.width()) return false;
	if (y >= image.height()) return false;

	foreach (QPoint p, points) {
		if (p.x() == x && p.y() == y) {
			// already visited
			return false;
		}

		if (qAbs(p.x() - x) > 3 && qAbs(p.y() - y) > 3) {
			// too far away from the start of the polygon
			break;
		}
	}


	for (int i = points.count() - 1; i >= 0; i--) {
		QPoint p = points.at(i);
		if (p.x() == x && p.y() == y) {
			// already visited
			return false;
		}

		if (qAbs(p.x() - x) > 3 && qAbs(p.y() - y) > 3) {
			// too far away from from the current point
			break;
		}
	}

	QRgb pixel = image.pixel(x, y);
    //DebugDialog::debug(QString("pixel %1,%2 %3").arg(x).arg(y).arg(pixel, 0, 16));
	if (pixel != 0xffffffff) {						
		// empty pixel, not on the border
		return false;
	}

	if (x + 1 == image.width()) {
		points.append(QPoint(x, y));
		return true;
	}

	pixel = image.pixel(x + 1, y);
	if (pixel != 0xffffffff) {						
		points.append(QPoint(x, y));
		return true;
	}

	if (y + 1 == image.height()) {
		points.append(QPoint(x, y));
		return true;
	}

	pixel = image.pixel(x, y + 1);
	if (pixel != 0xffffffff) {						
		points.append(QPoint(x, y));
		return true;
	}

	if (x - 1  < 0) {
		points.append(QPoint(x, y));
		return true;
	}

	pixel = image.pixel(x - 1, y);
	if (pixel != 0xffffffff) {						
		points.append(QPoint(x, y));
		return true;
	}

	if (y - 1  < 0) {
		points.append(QPoint(x, y));
		return true;
	}

	pixel = image.pixel(x, y - 1);
	if (pixel != 0xffffffff) {						
		points.append(QPoint(x, y));
		return true;
	}

	return false;
}

void GroundPlaneGenerator::setStrokeWidthIncrement(double swi) {
	m_strokeWidthIncrement = swi;
}

void GroundPlaneGenerator::setLayerName(const QString & layerName) {
	m_layerName = layerName;
}

const QString & GroundPlaneGenerator::layerName() {
	return m_layerName;
}

void GroundPlaneGenerator::setMinRunSize(int mrus, int mris) {
	m_minRunSize = mrus;
	m_minRiseSize = mris;
}

QString GroundPlaneGenerator::mergeSVGs(const QString & initialSVG, const QString & layerName) {
    QDomDocument doc;
    if (!initialSVG.isEmpty()) {
		TextUtils::mergeSvg(doc, initialSVG, layerName);        
    }
	foreach (QString newSvg, m_newSVGs) {
		TextUtils::mergeSvg(doc, newSvg, layerName);
	}
	return TextUtils::mergeSvgFinish(doc);
}

