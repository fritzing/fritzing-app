/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2019 Fritzing

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

********************************************************************/

#include "groundplanegenerator.h"
#include "../utils/graphicsutils.h"
#include "../utils/textutils.h"
#include "../processeventblocker.h"
#include "clipperhelpers.h"

#include <clipper.hpp>

#include <QBitArray>
#include <QFile>
#include <QPainter>
#include <QPaintDevice>
#include <QPaintEngine>
#include <QSvgRenderer>
#include <QDate>
#include <QTextStream>
#include <QtGlobal>
#include <qmath.h>

#include <boost/math/special_functions/relative_difference.hpp>
using boost::math::epsilon_difference;

#include <limits>
#include <QtConcurrentRun>

using namespace ClipperLib;


const QString GroundPlaneGenerator::KeepoutSettingName("GPG_Keepout");
const double GroundPlaneGenerator::KeepoutDefaultMils = 10;

static QList<Paths> convertCopperPolygonsToGroundPlane(Paths nonCopper, Paths thermalReliefPads, double pixelFactor, double keepoutMils, QPointF *seedPoint);

class GroundPlanePaintDevice;

class GroundPlanePaintEngine : public QPaintEngine {

public:
	GroundPlanePaintEngine() : QPaintEngine((QPaintEngine::PaintEngineFeatures) (QPaintEngine::AllFeatures
			& ~QPaintEngine::PatternBrush
			//& ~QPaintEngine::PainterPaths
			& ~QPaintEngine::PerspectiveTransform
			& ~QPaintEngine::ConicalGradientFill
			& ~QPaintEngine::PorterDuff)), clipperPaths() {
	}

	virtual bool begin(QPaintDevice *pdev) {
		return true;
	}

	virtual bool end() {
		return true;
	}

	virtual void updateState(const QPaintEngineState &state) {

	}

	virtual void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr) {

	}

	virtual void drawPath(const QPainterPath &path) override;

	virtual void drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode) override;

	virtual QPaintEngine::Type type() const {
		return User;
	}

	Paths grabCopper() {
		Clipper cp;
		Paths result;
		cp.AddPaths(clipperPaths, ptSubject, true);
		cp.Execute(ctUnion, result, pftNonZero, pftNonZero);
		return result;
	}

private:
	Paths clipperPaths;
};

class GroundPlanePaintDevice : public QPaintDevice {
public:
	GroundPlanePaintDevice(double physicalWidth_, double physicalHeight_, double dpi_)
			: QPaintDevice(), physicalWidth(physicalWidth_), physicalHeight(physicalHeight_), dpi(dpi_),
			  groundPlaneEngine(new GroundPlanePaintEngine()) {
	}

	~GroundPlanePaintDevice() {
		delete groundPlaneEngine;
	}

	virtual QPaintEngine *paintEngine() const {
		return groundPlaneEngine;
	}

	Paths grabCopper() const {
		return groundPlaneEngine->grabCopper();
	}

	double physicalWidth;
	double physicalHeight;
	double dpi;
protected:
	virtual int metric(QPaintDevice::PaintDeviceMetric metric) const {
		switch (metric) {
			case PdmWidth:
				return (int) (dpi * physicalWidth);
			case PdmHeight:
				return (int) (dpi * physicalHeight);
			case PdmDepth:
				return 1;
			case PdmNumColors:
				return 2;
			case PdmDpiX:
				return (int) dpi;
			case PdmDpiY:
				return (int) dpi;
			case PdmDevicePixelRatio:
				return 1;
			case PdmDevicePixelRatioScaled:
				return 1;
			default:
				qWarning("GroundPlanePaintDevice::metric() - metric %d unknown", metric);
				return 0;
		}
	}

private:
	GroundPlanePaintEngine *groundPlaneEngine;
};

QString GroundPlaneGenerator::ConnectorName = "connector0pad";
void saveClipperPathsToFile(Paths &paths, double clipperDPI, QString filename);

void GroundPlanePaintEngine::drawPath(const QPainterPath &path) {
	bool hasPen = state->pen().style() != Qt::NoPen;
	bool hasBrush = state->brush().style() != Qt::NoBrush;
	QList<QPolygonF> polygons = path.toSubpathPolygons();
	if (hasBrush) {
		Paths paths = polygonsToClipper(polygons, state->transform());
		Clipper cp;
		cp.AddPaths(clipperPaths, ptSubject, true);
		cp.AddPaths(paths, ptClip, true);
		cp.Execute(ctUnion, clipperPaths, pftNonZero, pftNonZero);
	}
	if (hasPen && state->pen().widthF() != 0) {
		QPainterPath stroke = QPainterPathStroker(state->pen()).createStroke(path);
		Paths strokePath = polygonsToClipper(stroke.toFillPolygons(), state->transform());
		Clipper cp;
		cp.AddPaths(clipperPaths, ptSubject, true);
		cp.AddPaths(strokePath, ptClip, true);
		cp.Execute(ctUnion, clipperPaths, pftNonZero, stroke.fillRule() == Qt::OddEvenFill ? pftEvenOdd : pftNonZero);
	}
}

void GroundPlanePaintEngine::drawPolygon(const QPointF *points, int pointCount, QPaintEngine::PolygonDrawMode mode) {
	bool hasPen = state->pen().style() != Qt::NoPen;
	bool hasBrush = state->brush().style() != Qt::NoBrush;
	Path path;
	Paths result;
	for (int i = 0; i < pointCount; i++) {
		const QPointF p2 = state->transform().map(points[i]);
		path << IntPoint((cInt) (p2.x()), (cInt) (p2.y()));
	}
	ClipperOffset co;
	co.AddPath(path, qtToClipperJoinType(state->pen().joinStyle()), qtToClipperEndType(state->pen().capStyle(), mode == QPaintEngine::PolylineMode, hasBrush));
	co.Execute(result, hasPen ? state->pen().widthF() / 2 / GraphicsUtils::StandardFritzingDPI * dynamic_cast<GroundPlanePaintDevice *>(paintDevice())->dpi : 0);
	Clipper cp;
	cp.AddPaths(clipperPaths, ptSubject, true);
	cp.AddPaths(result, ptClip, true);
	cp.Execute(ctUnion, clipperPaths, pftNonZero, qtToClipperFillType(mode));
}

GroundPlaneGenerator::GroundPlaneGenerator() {
	m_strokeWidthIncrement = 0;
	m_minRiseSize = m_minRunSize = 1;
}

GroundPlaneGenerator::~GroundPlaneGenerator() {
}

bool GroundPlaneGenerator::generateGroundPlaneUnit(const QString &boardSvg, QSizeF boardImageSize, const QString &svg, QSizeF copperImageSize,
		QStringList &exceptions, QGraphicsItem *board, double res, const QString &color, QPointF whereToStart, double keepoutMils) {

	QRectF bsbr = board->sceneBoundingRect();
	QPointF *s = new QPointF(res * (whereToStart.x() - bsbr.topLeft().x()) / GraphicsUtils::SVGDPI,
			res * (whereToStart.y() - bsbr.topLeft().y()) / GraphicsUtils::SVGDPI);

	GPGParams params;
	params.boardSvg = boardSvg;
	params.boardImageSize = boardImageSize;
	params.svg = svg;
	params.copperImageSize = copperImageSize;
	params.exceptions = exceptions;
	params.board = board;
	params.res = res;
	params.color = color;
	params.keepoutMils = keepoutMils;
	params.seedPoint = s;
	bool result = generateGroundPlaneFn(params);
	delete s;
	return result;
}

bool GroundPlaneGenerator::generateGroundPlane(const QString &boardSvg, QSizeF boardImageSize, const QString &svg, QSizeF copperImageSize,
		QStringList &exceptions, QGraphicsItem *board, double res, const QString &color, double keepoutMils, QList<GroundFillSeed> seeds) {

	GPGParams params;
	params.boardSvg = boardSvg;
	params.keepoutMils = keepoutMils;
	params.boardImageSize = boardImageSize;
	params.svg = svg;
	params.copperImageSize = copperImageSize;
	params.exceptions = exceptions;
	params.board = board;
	params.res = res;
	params.color = color;
	params.seeds = seeds;
	params.seedPoint = NULL;
	QFuture<bool> future = QtConcurrent::run(&GroundPlaneGenerator::generateGroundPlaneFn, this, params);
	while (!future.isFinished()) {
		ProcessEventBlocker::processEvents(200);
	}
	return future.result();
}

void saveClipperPathsToFile(Paths &paths, double clipperDPI, QString filename) {
	QFile f(filename);
	f.open(QFile::WriteOnly);
	QTextStream fs(&f);
	fs << clipperPathsToSVG(paths, clipperDPI);
	f.close();
}

bool GroundPlaneGenerator::generateGroundPlaneFn(const GPGParams & constParams) {
	GPGParams params = constParams;
	double bWidth, bHeight;
	double clipperDPI = params.res;
	Paths groundConnectorsZone;
	Paths groundThermalConnectors;
	createGroundThermalPads(params, clipperDPI, groundConnectorsZone, groundThermalConnectors);

	QRectF br = params.board->sceneBoundingRect();
	bWidth = br.width() / GraphicsUtils::SVGDPI;
	bHeight = br.height() / GraphicsUtils::SVGDPI;
	QSvgRenderer renderer(params.svg.toUtf8());
	QPainter painter;
	GroundPlanePaintDevice copperDevice(bWidth, bHeight, clipperDPI);
	painter.begin(&copperDevice);
	renderer.render(&painter);
	painter.end();
	Paths copper = copperDevice.grabCopper();
	GroundPlanePaintDevice boardDevice(bWidth, bHeight, clipperDPI);
	painter.begin(&boardDevice);
	QSvgRenderer(params.boardSvg.toUtf8()).render(&painter);
	painter.end();
	Paths board = boardDevice.grabCopper();

	Clipper cp;
	Paths copperWithoutGroundConnectors;
	Paths groundConnectors;
	cp.AddPaths(copper, ptSubject, true);
	cp.AddPaths(groundConnectorsZone, ptClip, true);
	cp.Execute(ctIntersection, groundConnectors, pftNonZero, pftNonZero);
	cp.Execute(ctDifference, copperWithoutGroundConnectors, pftNonZero, pftNonZero);

	cp.Clear();
	Paths nonCopper;
	cp.AddPaths(board, ptSubject, true);

	double BORDERMILS = 30;
	double extraOutlineClearance = BORDERMILS - params.keepoutMils;
	if (extraOutlineClearance > 0) {
		ClipperOffset co;
		Paths copperOutlineClearance;
		co.AddPaths(board, jtRound, ClipperLib::etClosedLine);
		co.Execute(copperOutlineClearance, extraOutlineClearance / 1000.0 * clipperDPI);
		cp.AddPaths(copperOutlineClearance, ptClip, true);
	}

	cp.AddPaths(copperWithoutGroundConnectors, ptClip, true);
	cp.Execute(ctDifference, nonCopper, pftNonZero, pftNonZero);

	ClipperOffset co;
	Paths expandedGroundConnectors;
	co.AddPaths(groundConnectors, jtRound, etClosedPolygon);
	co.Execute(expandedGroundConnectors, params.keepoutMils / 1000.0 * clipperDPI);

	cp.Clear();
	Paths thermalReliefPads;
	cp.AddPaths(expandedGroundConnectors, ptSubject, true);
	cp.AddPaths(groundThermalConnectors, ptClip, true);
	cp.Execute(ctDifference, thermalReliefPads, pftNonZero, pftNonZero);

	QList<Paths> groundCopper = convertCopperPolygonsToGroundPlane(nonCopper, thermalReliefPads, clipperDPI, params.keepoutMils, params.seedPoint);
	makeCopperFillFromPolygons(groundCopper, params.res, params.color, true, QSizeF(.05, .05), 1 / GraphicsUtils::SVGDPI);
	return true;
}

void GroundPlaneGenerator::createGroundThermalPads(GPGParams &params, double clipperDPI, Paths &groundConnectorsZone, Paths &groundThermalConnectors) {
	QTransform seedInflater;
	QSizeF clipperSize = params.boardImageSize * clipperDPI / GraphicsUtils::SVGDPI;
	seedInflater.scale(clipperSize.width(), clipperSize.height());
	double keepout = params.keepoutMils / 1000.0 * clipperDPI;
	for (int i = 0; i < params.seeds.size(); i++) {
		GroundFillSeed seed = params.seeds[i];
		QPolygonF scaledRect = seedInflater.map(seed.relativeRect);
		groundConnectorsZone << polygonToClipper(scaledRect, QTransform());
		QRectF bounds = scaledRect.boundingRect();
		QPointF center = bounds.center();
		double cx = center.x();
		double cy = center.y();
		double hw = bounds.width() / 2;
		double hh = bounds.height() / 2;
		double halfTraceWidth = std::max(std::min(hw, hh) / 3, keepout / 2);
		Path verticalRectangle;
		verticalRectangle
				<< IntPoint((cInt) (cx - halfTraceWidth), (cInt) (cy - hh - keepout))
				<< IntPoint((cInt) (cx + halfTraceWidth), (cInt) (cy - hh - keepout))
				<< IntPoint((cInt) (cx + halfTraceWidth), (cInt) (cy + hh + keepout))
				<< IntPoint((cInt) (cx - halfTraceWidth), (cInt) (cy + hh + keepout));
		Path verticalRectangle1;
		verticalRectangle1
				<< IntPoint((cInt) (cx - halfTraceWidth), (cInt) (cy - hh - keepout))
				<< IntPoint((cInt) (cx + halfTraceWidth), (cInt) (cy - hh - keepout))
				<< IntPoint((cInt) (cx + halfTraceWidth), (cInt) (cy - hh / 2))
				<< IntPoint((cInt) (cx - halfTraceWidth), (cInt) (cy - hh / 2));
		Path verticalRectangle2;
		verticalRectangle2
				<< IntPoint((cInt) (cx - halfTraceWidth), (cInt) (cy + hh / 2))
				<< IntPoint((cInt) (cx + halfTraceWidth), (cInt) (cy + hh / 2))
				<< IntPoint((cInt) (cx + halfTraceWidth), (cInt) (cy + hh + keepout))
				<< IntPoint((cInt) (cx - halfTraceWidth), (cInt) (cy + hh + keepout));
		Path horizontalRectangle1;
		horizontalRectangle1
				<< IntPoint((cInt) (cx + hw / 2), (cInt) (cy - halfTraceWidth))
				<< IntPoint((cInt) (cx + hw + keepout), (cInt) (cy - halfTraceWidth))
				<< IntPoint((cInt) (cx + hw + keepout), (cInt) (cy + halfTraceWidth))
				<< IntPoint((cInt) (cx + hw / 2), (cInt) (cy + halfTraceWidth));
		Path horizontalRectangle2;
		horizontalRectangle2
				<< IntPoint((cInt) (cx - hw - keepout), (cInt) (cy - halfTraceWidth))
				<< IntPoint((cInt) (cx - hw / 2), (cInt) (cy - halfTraceWidth))
				<< IntPoint((cInt) (cx - hw / 2), (cInt) (cy + halfTraceWidth))
				<< IntPoint((cInt) (cx - hw - keepout), (cInt) (cy + halfTraceWidth));
		groundThermalConnectors << verticalRectangle1 << verticalRectangle2 << horizontalRectangle1 << horizontalRectangle2;
	}
}

Paths findPolygonForPoint(PolyTree &tree, IntPoint seedPoint) {
	PolyNode *foundNode = NULL;
	for(int i = 0; i < tree.ChildCount(); i++) {
		if (PointInPolygon(seedPoint, tree.Childs[i]->Contour))
			foundNode = tree.Childs[i];
	}
	Paths polygon;
	if (foundNode) {
		polygon.push_back(foundNode->Contour);
		for (int i = 0; i < foundNode->ChildCount(); i++) {
			PolyNode *child = foundNode->Childs[i];
			polygon.push_back(child->Contour);
			/* for (int j = 0; j < child->ChildCount(); j++)
				 contours.append(child->Childs[j]);*/
		}
	}
	return polygon;
}

// this sorts a polygon tree to a list<(contour, hole1, hole2, ...)>
void sortPolygons(PolyTree &tree, QList<Paths> &polygons) {
	QList<PolyNode *> contours;
		foreach(PolyNode *initialNode, tree.Childs) {
			contours.append(initialNode);
		}
	while (contours.length()) {
		PolyNode *node = contours.takeFirst();
		Paths polygon;
		polygon.push_back(node->Contour);
		for (int i = 0; i < node->ChildCount(); i++) {
			PolyNode *child = node->Childs[i];
			polygon.push_back(child->Contour);
			for (int j = 0; j < child->ChildCount(); j++)
				contours.append(child->Childs[j]);
		}
		polygons.append(polygon);
	}
}

IntPoint findTopLeftMostPoint(Paths &paths) {
	IntPoint pt = paths[0][0];
	cInt leftDistance = pt.X + pt.Y;
	for (size_t j = 0; j < paths.size(); j++) {
		Path polygon = paths[0];
		for (size_t i = 0; i < polygon.size(); i++) {
			cInt dist = polygon[i].X + polygon[i].Y;
			if (dist < leftDistance) {
				leftDistance = dist;
				pt = polygon[i];
			}
		}
	}
	return pt;
}

QList<Paths> convertCopperPolygonsToGroundPlane(Paths nonCopper, Paths thermalReliefPads, double clipperDPI, double keepoutMils, QPointF *seedPoint) {
	Paths eroded, intermediate, nonCopperMinusKeepout;
	PolyTree groundFill;
	CleanPolygons(nonCopper);
	CleanPolygons(thermalReliefPads);

	ClipperOffset co;
	co.AddPaths(nonCopper, jtRound, etClosedPolygon);
	co.Execute(nonCopperMinusKeepout, -keepoutMils / 1000 * clipperDPI);

	double cappedKeepoutMils = std::max(keepoutMils / 4.0, 1.0);

	co.Clear();
	co.AddPaths(nonCopperMinusKeepout, jtRound, etClosedPolygon);
	co.Execute(intermediate, -cappedKeepoutMils / 1000 * clipperDPI);
	co.Clear();
	co.AddPaths(intermediate, jtRound, etClosedPolygon);
	co.Execute(eroded, cappedKeepoutMils  / 1000 * clipperDPI);
	CleanPolygons(eroded);

	Clipper clipper;
	clipper.AddPaths(eroded, ptSubject, true);
	clipper.AddPaths(thermalReliefPads, ptClip, true);
	clipper.Execute(ctDifference, groundFill, pftPositive, pftPositive);

	QList<Paths> sortedPolygons;
	if (seedPoint == NULL) {
		sortPolygons(groundFill, sortedPolygons);
	} else {
		sortedPolygons.append(findPolygonForPoint(groundFill, IntPoint((cInt) seedPoint->x(), (cInt) seedPoint->y())));
	}
	return sortedPolygons;
}

void GroundPlaneGenerator::makeCopperFillFromPolygons(QList<Paths> &sortedPolygons, double res,
		const QString &colorString, bool makeConnectorFlag, QSizeF minAreaInches, double minDimensionInches) {
	static const double standardConnectorWidth = .075;
	double targetDiameter = res * standardConnectorWidth;
	double targetDiameterAnd = targetDiameter * 1.25;
	double targetRadius = targetDiameter / 2;
	for(int k = 0; k < sortedPolygons.size(); k++) {
		Paths fragment = sortedPolygons[k];
		int minX = std::numeric_limits<int>::max();
		int minY = std::numeric_limits<int>::max();
		int maxX = std::numeric_limits<int>::min();
		int maxY = std::numeric_limits<int>::min();

		for (size_t i = 0; i < fragment.size(); i++) {
			for (size_t j = 0; j < fragment[i].size(); j++) {
				IntPoint pt = fragment[i][j];
				minX = std::min(minX, (int) pt.X);
				minY = std::min(minY, (int) pt.Y);
				maxX = std::max(maxX, (int) pt.X);
				maxY = std::max(maxY, (int) pt.Y);
			}
		}

		double xSpan = (maxX - minX) / res;
		double ySpan = (maxY - minY) / res;
		if ((xSpan < minAreaInches.width() && ySpan < minAreaInches.height()) || xSpan < minDimensionInches || ySpan < minDimensionInches)
			continue;
		double left = minX / res * GraphicsUtils::SVGDPI;
		double top = minY / res * GraphicsUtils::SVGDPI;

		QStringList pSvg(QString("<svg xmlns='http://www.w3.org/2000/svg' width='%1in' height='%2in' viewBox='0 0 %3 %4' >\n")
				.arg(xSpan)
				.arg(ySpan)
				.arg(maxX - minX)
				.arg(maxY - minY));
		pSvg << QString("<g id='%1'>\n").arg(m_layerName);
		pSvg << QString("<path fill='%1' stroke='none' stroke-width='0' d='").arg(colorString);
		for (size_t i = 0; i < fragment.size(); i++) {
			for (size_t j = 0; j < fragment[i].size(); j++) {
				IntPoint pt = fragment[i][j];
				pSvg << QString(j == 0 ? "M" : "L");
				pSvg << QString("%1,%2 ").arg(pt.X - minX).arg(pt.Y - minY);
			}
			pSvg << "Z";
		}
		pSvg << "'/>\n";
		if (makeConnectorFlag) {
			ClipperOffset co2(2.0, 10);
			Paths openedForConnector;
			co2.AddPaths(fragment, jtRound, etClosedPolygon);
			co2.Execute(openedForConnector, -targetDiameterAnd / 2);
			pSvg << QString("<g id='%1'>").arg(ConnectorName);
			if (openedForConnector.size()) {
				IntPoint pt = findTopLeftMostPoint(openedForConnector);
				pSvg << QString("<circle cx='%1' cy='%2' r='%3' fill='%4' stroke='none'/>")
						.arg(pt.X - minX)
						.arg(pt.Y - minY)
						.arg(targetRadius)
						.arg(colorString);
			} else {
				QString polyString = QString("<path fill='%1' stroke='none' stroke-width='0' d='").arg(colorString);
				if (fragment.size())
					for (size_t j = 0; j < fragment[0].size(); j++) {
						IntPoint pt = fragment[0][j];
						polyString += j == 0 ? "M" : "L";
						polyString += QString("%1,%2 ").arg(pt.X - minX).arg(pt.Y - minY);
					}
				polyString += "Z'/>\n";
				pSvg << polyString;
			}
			pSvg << QString("</g>\n");
		}
		pSvg << "</g>\n</svg>\n";
		m_newSVGs.append(pSvg.join(""));
		m_newOffsets.append(QPointF(left, top));
	}
}


const QStringList &GroundPlaneGenerator::newSVGs() {
	return m_newSVGs;
}

const QList<QPointF> &GroundPlaneGenerator::newOffsets() {
	return m_newOffsets;
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
	Q_FOREACH (QString newSvg, m_newSVGs) {
		TextUtils::mergeSvg(doc, newSvg, layerName);
	}
	return TextUtils::mergeSvgFinish(doc);
}
