/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2016 Fritzing

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

********************************************************************

$Revision: 6976 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-21 09:50:09 +0200 (So, 21. Apr 2013) $

********************************************************************/

#include <QMessageBox>
#include <QFileDialog>
#include <QSvgRenderer>
#include <qmath.h>
#include <set>
#include <utility>
#include <QDebug>
#include <QPaintEngine>
#include <cmath>
#include <QVector>

#include <QPaintDevice>
#include <QMultiHash>
#include <fstream>


#include "gerbergenerator.h"
#include "../debugdialog.h"
#include "../fsvgrenderer.h"
#include "../sketch/pcbsketchwidget.h"
#include "../connectors/connectoritem.h"
#include "../connectors/svgidlayer.h"
#include "svgfilesplitter.h"
#include "groundplanegenerator.h"
#include "../utils/graphicsutils.h"
#include "../utils/textutils.h"
#include "../utils/folderutils.h"
#include "../version/version.h"
#include "../lib/clipper/clipper.hpp"
#include "clipperhelpers.h"

using namespace ClipperLib;

const QRegExp GerberGenerator::MultipleZs("z\\s*[^\\s]");

const QString GerberGenerator::SilkTopSuffix = "_silkTop.gto";
const QString GerberGenerator::SilkBottomSuffix = "_silkBottom.gbo";
const QString GerberGenerator::CopperTopSuffix = "_copperTop.gtl";
const QString GerberGenerator::CopperBottomSuffix = "_copperBottom.gbl";
const QString GerberGenerator::MaskTopSuffix = "_maskTop.gts";
const QString GerberGenerator::MaskBottomSuffix = "_maskBottom.gbs";
const QString GerberGenerator::PasteMaskTopSuffix = "_pasteMaskTop.gtp";
const QString GerberGenerator::PasteMaskBottomSuffix = "_pasteMaskBottom.gbp";
const QString GerberGenerator::DrillSuffix = "_drill.txt";
const QString GerberGenerator::OutlineSuffix = "_contour.gm1";
const QString GerberGenerator::MagicBoardOutlineID = "boardoutline";

const double GerberGenerator::MaskClearanceMils = 5;
static const int dpi = 10000;
static const int gerberDecimals = 6;

class GerberPaintDevice;
class GerberPaintEngine : public QPaintEngine {
public:
    enum LayerType {
        Copper,
        SilkScreen,
        SolderMask,
        PasteStencil,
        Outline
    };
public:
    GerberPaintEngine(LayerType layerType_, double physicalWidth_, double physicalHeight_)
            : QPaintEngine((QPaintEngine::PaintEngineFeatures) (QPaintEngine::AllFeatures
                    & ~QPaintEngine::PatternBrush
                    & ~QPaintEngine::PerspectiveTransform
                    & ~QPaintEngine::ConicalGradientFill
                    & ~QPaintEngine::PorterDuff)),
              regions(),
              roundApertures(),
              layerType(layerType_),
              physicalWidth(physicalWidth_),
              physicalHeight(physicalHeight_),
              sendEverythingToClipper(layerType_ == SolderMask || layerType_ == Outline),
              clipperPaths(),
              solderMask(),
              outline(),
              apertureIndex(11) {
    }

    virtual void drawEllipse(const QRectF &r) override;

    virtual void drawRects(const QRectF *rects, int rectCount) override;

    virtual void drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode) override;

    virtual void drawPath(const QPainterPath &path) override;

    virtual bool begin(QPaintDevice *pdev) {
        return true;
    }

    virtual bool end() {
        return true;
    }

    void collectFile(QTextStream &output, bool mirrorX = false);
    bool isEmpty() {
        return clipperPaths.size() == 0 && roundApertures.size() == 0 && rectApertures.size() == 0;
    }

    virtual void updateState(const QPaintEngineState &state) {
    };

    virtual void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr){
        qWarning("drawPixmap");
    };

    virtual Type type() const{
        return User;
    }

    Paths getOutline() {
        return outline;
    }

    void setLayerType(LayerType layerType_) {
        if (layerType == Outline)
            outline.swap(clipperPaths);
        if (layerType == SolderMask) {
            solderMask.clear();
            Clipper cp;
            cp.AddPaths(outline, ptSubject, true);
            cp.AddPaths(clipperPaths, ptClip, true);
            cp.Execute(ctDifference, solderMask, pftNonZero, pftNonZero);
        }
        clipperPaths.clear();
        regions.clear();
        roundApertures.clear();
        rectApertures.clear();
        layerType = layerType_;
        sendEverythingToClipper = layerType_ == SolderMask || layerType_ == Outline || layerType_ == SilkScreen;
    }

    struct RectAperture {
        RectAperture(int id_):id(id_), w(0), h(0), holeX(0), holeY(0) {}
        double w;
        double h;
        double holeX;
        double holeY;
        int id;

        void toDefinition(QTextStream &stream) const{
            stream << "%ADD" << QString::number(id) << "R," << QString::number(w) << "X" << QString::number(h);
            if (holeX) {
                stream << "X" << QString::number(holeX) ;
                if (holeY)
                    stream << "X" << QString::number(holeY);
            }
            stream << "*%\n";
        }

        bool operator==(const RectAperture &e2) const
        {
            return w == e2.w && h == e2.h && holeX == e2.holeX && holeY == e2.holeY;
        }
    };

    struct RoundAperture {
        RoundAperture(int id_):id(id_), diameter(0), holeX(0), holeY(0) {}
        double diameter;
        double holeX;
        double holeY;
        int id;

        void toDefinition(QTextStream &stream) const{
            stream << "%ADD" << QString::number(id) << "C," << QString::number(diameter);
            // don't be fooled, in gerber "X" is a separator, not an axis name.
            if (holeX) {
                stream << "X" << QString::number(holeX) ;
                if (holeY)
                    stream << "X" << QString::number(holeY);
            }
             stream << "*%\n";
        }

        bool operator==(const RoundAperture &e2) const
        {
            return diameter == e2.diameter && holeX == e2.holeX && holeY == e2.holeY;
        }
    };

    struct GerberRegion {
        GerberRegion(Path polygon_, bool lightPolarity_):polygon(polygon_), lightPolarity(lightPolarity_) {
        }
        Path polygon;
        bool lightPolarity;
    };

protected:
    void xyd(QTextStream &stream, double x, double y, int d=3, bool mirrorX = false) {
        const double gerberFactor = std::pow(10.0, gerberDecimals) / dpi;

        double h = physicalHeight * dpi;
        double xInDPI = mirrorX ? (physicalWidth * dpi - x) : x;
        stream << "X" << QString::number(qRound(xInDPI * gerberFactor))
                << "Y" << QString::number(qRound((h - y) * gerberFactor))
                << "D0" << QString::number(d) << "*\n";
    }
    int nextId() {
        return apertureIndex++;
    }

    bool wouldBeClipped(QPolygonF polygon, double penWidth) {
        double adjustmentFactor = penWidth / 2 * dpi;
        QRectF bounds = polygon.boundingRect().adjusted(adjustmentFactor, adjustmentFactor, adjustmentFactor, adjustmentFactor);
        Clipper cp;
        Paths extraMeat;
        Path path = polygonToClipper(QMatrix().map(bounds));
        cp.AddPath(path, ptSubject, true);
        if (layerType == SilkScreen && solderMask.size())
            cp.AddPaths(solderMask, ptClip, true);
        else
            cp.AddPaths(outline, ptClip, true);
        cp.Execute(ctDifference, extraMeat, pftNonZero, pftNonZero);
        return extraMeat.size() != 0;
    }

    void accumulateClipper(Paths &paths) {
        Clipper cp;
        Paths clipped;
        if (layerType == SilkScreen && solderMask.size()){
            cp.AddPaths(paths, ptSubject, true);
            cp.AddPaths(solderMask, ptClip, true);
            cp.Execute(ctIntersection, clipped, pftNonZero, pftNonZero);
        } else {
            if (layerType != Outline && outline.size()) {
                cp.AddPaths(outline, ptSubject, true);
                cp.AddPaths(paths, ptClip, true);
                cp.Execute(ctIntersection, clipped, pftNonZero, pftNonZero);
            } else
                clipped = paths;
        }
        clipperPaths.insert(clipperPaths.end(), clipped.begin(), clipped.end());
    }

private:
    QList<GerberRegion> regions;
    Paths outline;
    Paths solderMask;
    Paths clipperPaths;
    QMultiHash<RoundAperture, QList<QPointF> > roundApertures;
    QMultiHash<RectAperture, QPointF> rectApertures;
    LayerType layerType;
    double physicalWidth;
    double physicalHeight;
    int apertureIndex;
    bool sendEverythingToClipper;
};

class GerberPaintDevice : public QPaintDevice {

public:
    GerberPaintDevice(double physicalWidth_, double physicalHeight_, GerberPaintEngine::LayerType layerType_)
            : QPaintDevice()
            , physicalWidth(physicalWidth_)
            , physicalHeight(physicalHeight_)
            , gerberEngine(layerType_, physicalWidth_, physicalHeight_)
            , gerberEnginePtr(&gerberEngine) {
    }

    virtual QPaintEngine *paintEngine() const {
        return gerberEnginePtr;
    };
    GerberPaintEngine *gerberPaintEngine() {
        return gerberEnginePtr;
    }

    double physicalWidth;
    double physicalHeight;
private:
    GerberPaintEngine gerberEngine;
    GerberPaintEngine *gerberEnginePtr;
protected:
    virtual int metric(QPaintDevice::PaintDeviceMetric metric) const {
        switch(metric) {
            case PdmWidth:
                return (int) (dpi * physicalWidth);
            case PdmHeight:
                return (int) (dpi * physicalHeight);
            case PdmDepth:
                return 1;
            case PdmNumColors:
                return 2;
            case PdmDpiX:
                return dpi;
            case PdmDpiY:
                return dpi;
            case PdmDevicePixelRatio:
                return 1;
            default:
                qWarning("GerberPaintDevice::metric() - metric %d unknown", metric);
                return 0;
        }
    }
};
class HolePaintDevice;

class HolePaintEngine : public QPaintEngine {

public:
    HolePaintEngine()
            : QPaintEngine((QPaintEngine::PaintEngineFeatures) (QPaintEngine::AllFeatures
                    & ~QPaintEngine::Antialiasing
                    & ~QPaintEngine::PainterPaths
                    & ~QPaintEngine::PatternBrush
                    & ~QPaintEngine::PerspectiveTransform
                    & ~QPaintEngine::ConicalGradientFill
                    & ~QPaintEngine::PorterDuff)) {

    };
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

    virtual QPaintEngine::Type type() const {
        return User;
    }

    virtual void drawEllipse(const QRectF &r) override;

    virtual void drawPolygon(const QPointF *points, int pointCount, QPaintEngine::PolygonDrawMode mode) override {
        //pass
    }
};

class HolePaintDevice : public QPaintDevice {
public:
    HolePaintDevice(double physicalWidth_, double physicalHeight_)
            :physicalWidth(physicalWidth_)
            , physicalHeight(physicalHeight_)
            , holePaintEngine(new HolePaintEngine())
            , holes(){
    };

    ~HolePaintDevice() {
        delete holePaintEngine;
    }

    QPaintEngine *paintEngine() const {
        return holePaintEngine;
    }

    void addHole(QPointF center, double diameter) {
        //in the SVG, the same hole could be declared as various diameter circles, so we only keep the biggest diameter for each center
        QHash<QPointF, double>::iterator previousDiameter = centerToDiameter.find(center);
        if (previousDiameter != centerToDiameter.end()) {
            if (*previousDiameter < diameter) {
                holes.find(diameter)->remove(center);
            } else
                return;
        }
        centerToDiameter.insert(center, diameter);
        QMap<double, QSet<QPointF> >::iterator set = holes.find(diameter);
        if (set != holes.end())
            set->insert(center);
        else {
            QSet<QPointF> newSet;
            newSet.insert(center);
            holes.insert(diameter, newSet);
        }
    }

    QMap<double, QSet<QPointF> > getHoles() {
        return holes;
    }
    void clearHoles() {
        holes.clear();
        centerToDiameter.clear();
    }

private:
    HolePaintEngine *holePaintEngine;
    double physicalWidth;
    double physicalHeight;
    QMap<double, QSet<QPointF> > holes;
    QHash<QPointF, double> centerToDiameter;
protected:
    virtual int metric(QPaintDevice::PaintDeviceMetric metric) const {
        switch(metric) {
            case PdmWidth:
                return (int) (dpi * physicalWidth);
            case PdmHeight:
                return (int) (dpi * physicalHeight);
            case PdmDepth:
                return 1;
            case PdmNumColors:
                return 2;
            case PdmDpiX:
                return dpi;
            case PdmDpiY:
                return dpi;
            case PdmDevicePixelRatio:
                return 1;
            default:
                qWarning("GerberPaintDevice::metric() - metric %d unknown", metric);
                return 0;
        }
    }
};
////////////////////////////////////////////

#if QT_VERSION < QT_VERSION_CHECK(5, 3, 0)
    //stolen from Qt 5.3
    static inline uint hash(const uchar *p, int len, uint seed) Q_DECL_NOTHROW
    {
        uint h = seed;
        for (int i = 0; i < len; ++i)
            h = 31 * h + p[i];
        return h;
    }

    //stolen from Qt 5.3
    uint qHash(double key, uint seed=0) Q_DECL_NOTHROW
    {
        return key != 0.0  ? hash(reinterpret_cast<const uchar *>(&key), sizeof(key), seed) : seed;
    }
#endif
uint qHash(const GerberPaintEngine::RectAperture &key) {
    return qHash(key.w) ^ qHash(key.h) ^ qHash(key.holeX) ^ qHash(key.holeY);
}
uint qHash(const GerberPaintEngine::RoundAperture &key) {
    return qHash(key.diameter) ^ qHash(key.holeX) ^ qHash(key.holeY);
}
uint qHash(const QPointF &key) {
    return qHash(key.x()) ^ qHash(key.y());
}
void HolePaintEngine::drawEllipse(const QRectF &r) {
    if (r.width() != r.height())
        return;
    QPointF center = state->matrix().map(r.center());
    double diameter = r.width();
    if (state->pen().style() != Qt::NoPen != state->pen().widthF() != 0){
        diameter -= state->pen().widthF();
    }
    diameter /= GraphicsUtils::StandardFritzingDPI;
    HolePaintDevice *device = dynamic_cast<HolePaintDevice *>(paintDevice());
    device->addHole(center / dpi, diameter);
}

// we collect the polygon hierarchy from outside to inside
// to give gerber the correct order: the big polygon, then its holes, then the smaller polygons that go in the holes etc.
static void recursivelyCollectPolygons(PolyNode *currentNode, QList<GerberPaintEngine::GerberRegion> &regions) {
    for (int i = 0; i < currentNode->ChildCount(); i++) {
        PolyNode *child = currentNode->Childs[i];
        regions.append(GerberPaintEngine::GerberRegion(child->Contour, false));
        for (int j = 0; j < child->ChildCount(); j++) {
            regions.append(GerberPaintEngine::GerberRegion(child->Childs[j]->Contour, true));
            recursivelyCollectPolygons(child->Childs[j], regions);
        }
    }
}

void GerberPaintEngine::collectFile(QTextStream &output, bool mirrorX) {
    output << "%ASAXBY*%\n"
            << QString("%FSLAX2%1Y2%2*%\n").arg(gerberDecimals).arg(gerberDecimals)
            << "%MOIN*%\n"
            << "%OFA0B0*%\n"
            << "%SFA1.0B1.0*%\n"
            << "G01*\n";
    PolyTree result;
    if (layerType == SolderMask) {
        Paths unions;
        Clipper cp;
        cp.AddPaths(clipperPaths, ptSubject, true);
        cp.Execute(ctUnion, unions, pftNonZero, pftNonZero);

        ClipperOffset co;
        co.AddPaths(unions, jtRound, etClosedPolygon);
        co.Execute(result, GerberGenerator::MaskClearanceMils / 1000.0 * dpi );
    } else {
        Clipper cp;
        cp.AddPaths(clipperPaths, ptSubject, true);
        cp.Execute(ctUnion, result, pftNonZero, pftNonZero);
    }
    clipperPaths.clear();
    ClosedPathsFromPolyTree(result, clipperPaths);
    recursivelyCollectPolygons(&result, regions);
    if (layerType == Outline) {
        RoundAperture a(nextId());
        a.diameter = 0;
        for (int i = 0; i < regions.size(); i++) {
            GerberRegion r = regions[i];
            QList<QPointF> list;
            for (size_t j = 0; j < r.polygon.size(); j++)
                list << QPointF(r.polygon[j].X, r.polygon[j].Y);
            list << QPointF(r.polygon[0].X, r.polygon[0].Y);
            roundApertures.insert(a, list);
        }
    }
    QList<RoundAperture> roundAperturesDefs = roundApertures.uniqueKeys();
    for(QList<RoundAperture>::iterator i = roundAperturesDefs.begin(); i != roundAperturesDefs.end(); ++i) {
        i->toDefinition(output);
    }
    QList<RectAperture> rectAperturesDefs = rectApertures.uniqueKeys();
    for(QList<RectAperture>::iterator i = rectAperturesDefs.begin(); i != rectAperturesDefs.end(); ++i) {
        i->toDefinition(output);
    }
    if (regions.size()) {
        if (layerType != Outline) {
            for (int i = 0; i < regions.size(); i++) {
                GerberRegion r = regions[i];
                if (r.lightPolarity)
                    output << "%LPC*%\n";
                output << "G36*\n";
                for (size_t j = 0; j < r.polygon.size(); j++)
                    xyd(output, r.polygon[j].X, r.polygon[j].Y, j == 0 ? 2 : 1, mirrorX);
                if (r.lightPolarity)
                    output << "%LPD*%\n";
            }
            output << "G37*\n";
        }
    }
    for(QList<RoundAperture>::iterator i = roundAperturesDefs.begin(); i != roundAperturesDefs.end(); ++i) {
        output << "D" << QString::number(i->id) << "*\n";
        for(QMultiHash<RoundAperture, QList<QPointF> >::iterator j = roundApertures.find(*i); j != roundApertures.end() && j.key() == *i; ++j) {
            const QList<QPointF> &pointList = j.value();
            if (pointList.size() == 1)
                xyd(output, pointList[0].x(), pointList[0].y(), 3, mirrorX);
            else
                for (int k = 0; k < pointList.size(); k++)
                    xyd(output, pointList[k].x(), pointList[k].y(), k == 0 ? 2 : 1, mirrorX);
        }
    }
    for(QList<RectAperture>::iterator i = rectAperturesDefs.begin(); i != rectAperturesDefs.end(); ++i) {
        output << "D" << QString::number(i->id) << "*\n";
        for(QMultiHash<RectAperture, QPointF>::iterator j = rectApertures.find(*i); j != rectApertures.end(); ++j)
            xyd(output, j.value().x(), j.value().y(), 3, mirrorX);
    }
    output << "M02*\n";
}


void GerberPaintEngine::drawPath(const QPainterPath &path) {
    bool hasPen = state->pen().style() != Qt::NoPen;
    bool hasBrush = state->brush().style() != Qt::NoBrush;
    QList<QPolygonF> polygons = path.toSubpathPolygons();
    if (hasBrush) {
        Paths paths = polygonsToClipper(polygons, state->matrix());
        accumulateClipper(paths);
    }
    if (hasPen && state->pen().widthF() != 0)
        for(int i = 0; i < polygons.size(); i++){
            QPolygonF poly = polygons[i];
            PolygonDrawMode drawMode = poly.isClosed() ? (path.fillRule() == Qt::OddEvenFill ? QPaintEngine::OddEvenMode : QPaintEngine::WindingMode) : QPaintEngine::PolylineMode;
            drawPolygon(poly.data(), poly.size(), drawMode);
        }
}

void GerberPaintEngine::drawPolygon(const QPointF *points, int pointCount, QPaintEngine::PolygonDrawMode mode) {
    bool hasPen = state->pen().style() != Qt::NoPen && state->pen().widthF() != 0.0;
    bool hasBrush = state->brush().style() != Qt::NoBrush;
    if (!(hasPen || hasBrush))
        return;
    if (!sendEverythingToClipper && mode == QPaintEngine::PolylineMode) {
        QList<QPointF> list;
        double penwidth = state->pen().widthF() / GraphicsUtils::StandardFritzingDPI;
        for (int i = 0; i < pointCount; i++)
            list << (state->matrix().map(points[i]));
        if (!wouldBeClipped(QPolygonF(QVector<QPointF>::fromList(list)), penwidth)) {
            RoundAperture a(nextId());
            a.diameter = penwidth;
            roundApertures.insert(a, list);
            return;
        }
    }
    Path path(pointsToClipper(points, pointCount, state->matrix()));
    Paths result;
    ClipperOffset co;
    co.AddPath(path, qtToClipperJoinType(state->pen().joinStyle()), qtToClipperEndType(state->pen().capStyle(), mode == QPaintEngine::PolylineMode, hasBrush));
    co.Execute(result, hasPen ? state->pen().widthF() / 2 / GraphicsUtils::StandardFritzingDPI * dpi : 0);
    accumulateClipper(result);
}

void GerberPaintEngine::drawEllipse(const QRectF &r) {
    bool hasPen = state->pen().style() != Qt::NoPen;
    bool hasBrush = state->brush().style() != Qt::NoBrush;
    if (!(hasPen || hasBrush))
        return;
    if (layerType == GerberPaintEngine::SolderMask){
        painter()->save();
        painter()->setBrush(Qt::black);
    }
    if (sendEverythingToClipper || r.width() != r.height())
       QPaintEngine::drawEllipse(r);
    else {
        RoundAperture a(nextId());
        double penwidth = hasPen ? state->pen().widthF() : 0;
        a.diameter = (r.width() + penwidth) / GraphicsUtils::StandardFritzingDPI;
        if (layerType == GerberPaintEngine::Copper || layerType == GerberPaintEngine::SilkScreen) {
            qreal holeSize = r.width() - penwidth;
            //this happens when the pen.widthF is larger than the circle diameter
            if (holeSize < 0)
                holeSize = 0;
            a.holeX = hasBrush ? 0 : holeSize / GraphicsUtils::StandardFritzingDPI;
        }
        if (wouldBeClipped(state->matrix().map(r), penwidth))
            QPaintEngine::drawEllipse(r);
        else
            roundApertures.insert(a, QList<QPointF>() << state->matrix().map(r.center()));
    }
    if (layerType == GerberPaintEngine::SolderMask)
        painter()->restore();
}

static double roundOnGrid(double value, double gridSize) {
    return round(value * gridSize) / gridSize;
}

void GerberPaintEngine::drawRects(const QRectF *rects, int rectCount) {
    bool hasPen = state->pen().style() != Qt::NoPen;
    bool hasBrush = state->brush().style() != Qt::NoBrush;
    if (!(hasPen || hasBrush))
        return;
    if (layerType == GerberPaintEngine::SolderMask){
        //paint the holes in solder mask
        painter()->save();
        painter()->setBrush(Qt::black);
    }
    if (sendEverythingToClipper)
         QPaintEngine::drawRects(rects, rectCount);
    else {
        double penWidth = hasPen ? state->pen().widthF() / GraphicsUtils::StandardFritzingDPI : 0;
        for (int i = 0; i < rectCount; i++) {
            const QPolygonF poly  = state->matrix().map(rects[i]);
            //now check is the polygon is still aligned with the axes by checking if one edge is the same length as a bounding rectangle edge
            const QRectF rect = poly.boundingRect();
            const QPointF edge = poly[0] - poly[1];
            const double edgeL = edge.x() * edge.x() + edge.y() * edge.y();
            const double wSq = rect.width() * rect.width();
            const double hSq = rect.height() * rect.height();
            // we use relative difference because the matrix computation might have scatered the resolution
            if ((wSq && fabs((edgeL - wSq) / wSq) < 1 / dpi / 20) || (hSq && fabs((edgeL - hSq) / hSq) < 1 / dpi / 20)) {
                RectAperture a(nextId());
                const double gridResolution = dpi * 10;
                a.w = roundOnGrid(rect.width() / dpi + penWidth, gridResolution);
                a.h = roundOnGrid(rect.height() / dpi + penWidth, gridResolution);
                if (layerType == GerberPaintEngine::Copper || layerType == GerberPaintEngine::SilkScreen) {
                    if (!hasBrush) {
                        a.holeX = roundOnGrid(rect.width() / dpi - penWidth, gridResolution);
                        a.holeY = roundOnGrid(rect.height() / dpi - penWidth, gridResolution);
                    }
                }
                if (wouldBeClipped(poly, penWidth))
                    QPaintEngine::drawRects(rects + i, 1);
                else
                    rectApertures.insert(a, rect.center());
            } else
                QPaintEngine::drawRects(rects + i, 1);
        }
    }
    if (layerType == GerberPaintEngine::SolderMask)
        painter()->restore();
}

////////////////////////////////////////////

void collectGerberFile(GerberPaintEngine *engine, const QString &exportDir, const QString &prefix, const QString &suffix, bool mirrorX = false) {
    if (engine->isEmpty())
        return;
    QString outname = exportDir + "/" +  prefix + suffix;
    QFile file(outname);
    file.open(QIODevice::WriteOnly);
    QTextStream out(&file);
    engine->collectFile(out);
    file.close();
    if (mirrorX) {
        QFile file2(exportDir + "/" +  prefix + "_mirror" + suffix);
        file2.open(QIODevice::WriteOnly);
        QTextStream out2(&file2);
        engine->collectFile(out2, true);
        file2.close();
    }
}

void svgToGerberFile(const QString &svg, GerberPaintEngine::LayerType layerType, const QString &exportDir, const QString &prefix, const QString &suffix, bool mirrorX = false) {
    QSizeF svgSize = TextUtils::parseForWidthAndHeight(svg);
    GerberPaintDevice device(svgSize.width(), svgSize.height(), layerType);
    QSvgRenderer renderer(svg.toUtf8());
    QPainter painter;
    painter.begin(&device);
    renderer.render(&painter);
    painter.end();
    collectGerberFile(device.gerberPaintEngine(), exportDir, prefix, suffix, mirrorX);
}


void GerberGenerator::exportToGerber(const QString & prefix, const QString & exportDir, ItemBase * board, PCBSketchWidget * sketchWidget, bool displayMessageBoxes)
{
	if (board == NULL) {
        int boardCount;
		board = sketchWidget->findSelectedBoard(boardCount);
		if (boardCount == 0) {
			DebugDialog::debug("board not found");
			return;
		}
		if (board == NULL) {
			DebugDialog::debug("multiple boards found");
			return;
		}
	}

    exportPickAndPlace(prefix, exportDir, board, sketchWidget, displayMessageBoxes);

    // now do it for the outline/contour
    LayerList outlineLayerIDs = ViewLayer::outlineLayers();
    QString svgOutline = renderTo(outlineLayerIDs, board, sketchWidget);
    if (svgOutline.isEmpty()) {
        displayMessage(QObject::tr("outline is empty"), displayMessageBoxes);
        return;
    }

    QSizeF svgSize = TextUtils::parseForWidthAndHeight(svgOutline);
    GerberPaintDevice device(svgSize.width(), svgSize.height(), GerberPaintEngine::Outline);
    QPainter painter;
    painter.begin(&device);

    //outline
    device.gerberPaintEngine()->setLayerType(GerberPaintEngine::Outline);
    svgOutline = cleanOutline(svgOutline);
    QSvgRenderer(svgOutline.toUtf8()).render(&painter);
    collectGerberFile(device.gerberPaintEngine(), exportDir, prefix, OutlineSuffix);

    //copper Bottom
    device.gerberPaintEngine()->setLayerType(GerberPaintEngine::Copper);
    QSvgRenderer(renderTo(ViewLayer::copperLayers(ViewLayer::NewBottom), board, sketchWidget).toUtf8()).render(&painter);
    collectGerberFile(device.gerberPaintEngine(), exportDir, prefix, CopperBottomSuffix, true);

    //copper Top
    device.gerberPaintEngine()->setLayerType(GerberPaintEngine::Copper);
    QSvgRenderer(renderTo(ViewLayer::copperLayers(ViewLayer::NewTop), board, sketchWidget).toUtf8()).render(&painter);
    collectGerberFile(device.gerberPaintEngine(), exportDir, prefix, CopperTopSuffix, false);

    //solder mask Bottom
    QList<ItemBase *> copperLogoItems;
    sketchWidget->hideCopperLogoItems(copperLogoItems);
    device.gerberPaintEngine()->setLayerType(GerberPaintEngine::SolderMask);
    QSvgRenderer(renderTo(ViewLayer::maskLayers(ViewLayer::NewBottom), board, sketchWidget).toUtf8()).render(&painter);
    collectGerberFile(device.gerberPaintEngine(), exportDir, prefix, MaskBottomSuffix, true);
    sketchWidget->restoreCopperLogoItems(copperLogoItems);

    //solder paste stencil Bottom
    collectGerberFile(device.gerberPaintEngine(), exportDir, prefix, PasteMaskBottomSuffix, true);

    //silk screen Bottom
    device.gerberPaintEngine()->setLayerType(GerberPaintEngine::SilkScreen);
    QSvgRenderer(renderTo(ViewLayer::silkLayers(ViewLayer::NewBottom), board, sketchWidget).toUtf8()).render(&painter);
    collectGerberFile(device.gerberPaintEngine(), exportDir, prefix, SilkBottomSuffix, true);

    //solder mask Top
    QList<ItemBase *> copperLogoItemsTop;
    sketchWidget->hideCopperLogoItems(copperLogoItemsTop);
    device.gerberPaintEngine()->setLayerType(GerberPaintEngine::SolderMask);
    QSvgRenderer(renderTo(ViewLayer::maskLayers(ViewLayer::NewTop), board, sketchWidget).toUtf8()).render(&painter);
    collectGerberFile(device.gerberPaintEngine(), exportDir, prefix, MaskTopSuffix, false);
    sketchWidget->restoreCopperLogoItems(copperLogoItemsTop);

    //solder paste stencil Top
    collectGerberFile(device.gerberPaintEngine(), exportDir, prefix, PasteMaskTopSuffix, false);

    //silk screen Top
    device.gerberPaintEngine()->setLayerType(GerberPaintEngine::SilkScreen);
    QSvgRenderer(renderTo(ViewLayer::silkLayers(ViewLayer::NewTop), board, sketchWidget).toUtf8()).render(&painter);
    collectGerberFile(device.gerberPaintEngine(), exportDir, prefix, SilkTopSuffix, false);

    painter.end();
    doDrill(board, sketchWidget, prefix, exportDir, device.gerberPaintEngine()->getOutline());
}

void GerberGenerator::exportFile(const QString &svg, const QString &layerName, SVG2gerber::ForWhy forWhy, const QString &exportDir,
        const QString &prefix, const QString &suffix) {

    switch(forWhy){

        case SVG2gerber::ForCopper:
            svgToGerberFile(svg, GerberPaintEngine::Copper, exportDir, prefix, suffix);
            break;
        case SVG2gerber::ForSilk:
            svgToGerberFile(svg, GerberPaintEngine::SilkScreen, exportDir, prefix, suffix);
            break;
        case SVG2gerber::ForOutline:
            svgToGerberFile(svg, GerberPaintEngine::Outline, exportDir, prefix, suffix);
            break;
        case SVG2gerber::ForMask:
            svgToGerberFile(svg, GerberPaintEngine::SolderMask, exportDir, prefix, suffix);
            break;
        case SVG2gerber::ForDrill: {
            Paths paths;
            GerberGenerator::svgToExcellon(prefix, exportDir, svg, paths);
        }
            break;
        case SVG2gerber::ForPasteMask:
            svgToGerberFile(svg, GerberPaintEngine::PasteStencil, exportDir, prefix, suffix);
            break;
    }
}

static QDomDocument extractHoles(bool plated, QString svgDrill) {
    QDomDocument holesDocument = QDomDocument("svg");
    holesDocument.setContent(svgDrill);
    QDomNodeList circleList = holesDocument.elementsByTagName("circle");
    for(int i =circleList.count() - 1; i >=0 ; i--) {
        QDomElement node = circleList.item(i).toElement();
        if (plated == (node.attribute("id").contains(FSvgRenderer::NonConnectorName) && node.attribute("stroke-width").toDouble() == 0))
            node.parentNode().removeChild(node);
    }
    return holesDocument;
}

static bool pointInPolygon(Paths &paths, IntPoint point) {
    for(int i = 0; i < paths.size(); i++)
        if (Orientation(paths[i]) != (PointInPolygon(point, paths[i])==1))
            return false;
    return true;
}

static void dumpHoles(QStringList &excellonHeader,QStringList &excellonBody, const QMap<double, QSet<QPointF> > holes, int holeIndex, double height, const Paths &outline) {
    ClipperOffset co;
    co.AddPaths(outline, jtRound, etClosedPolygon);
    for (QMap<double, QSet<QPointF> >::const_iterator it = holes.begin(); it != holes.end(); ++it) {
        Paths offsetOutline;
        co.Execute(offsetOutline, -it.key() / 2.0 * dpi);
        excellonHeader << "T" << QString::number(holeIndex) << "C" << QString::number(it.key()) << "\n";
        excellonBody << "T" << QString::number(holeIndex) << "\n";
        for(QSet<QPointF>::const_iterator it2 = it->begin(); it2 != it->end(); ++it2)
            if (pointInPolygon(offsetOutline, IntPoint((cInt) (it2->x() * dpi), (cInt) (it2->y() * dpi))))
                excellonBody
                        << "X" << QString("%1").arg(qRound(it2->x() * 10000), 6, 10, QChar('0'))
                        << "Y" << QString("%1").arg(qRound((height - it2->y()) * 10000), 6, 10, QChar('0')) << "\n";
        holeIndex++;
    }
}

int GerberGenerator::doDrill(ItemBase *board, PCBSketchWidget *sketchWidget, const QString &filename, const QString &exportDir, const Paths &outline)
{
    LayerList drillLayerIDs;
    drillLayerIDs << ViewLayer::drillLayers();
    QString svgDrill = renderTo(drillLayerIDs, board, sketchWidget);
    svgToExcellon(filename, exportDir, svgDrill, outline);
    return 0;
}

void GerberGenerator::svgToExcellon(QString const &filename, QString const &exportDir,const QString &svgDrill, const Paths &outline) {
    QSizeF svgSize = TextUtils::parseForWidthAndHeight(svgDrill);
    QDomDocument platedHolesDocument = extractHoles(true, svgDrill);
    QDomDocument nonPlatedHolesDocument = extractHoles(false, svgDrill);

    HolePaintDevice device(svgSize.width(),  svgSize.height());
    QSvgRenderer platedRenderer(platedHolesDocument.toByteArray());
    QSvgRenderer nonPlatedRenderer(nonPlatedHolesDocument.toByteArray());
    QPainter painter;
    painter.begin(&device);
    platedRenderer.render(&painter);
    QMap<double, QSet<QPointF> > platedHoles = device.getHoles();
    device.clearHoles();
    nonPlatedRenderer.render(&painter);
    QMap<double, QSet<QPointF> > nonPlatedHoles = device.getHoles();
    device.clearHoles();
    painter.end();

    QStringList excellonHeader;
    QStringList excellonBody;

    int initialHoleIndex = 1;
    int initialPlatedIndex = std::max(101, nonPlatedHoles.size() + 1);

    excellonHeader << QString("; NON-PLATED HOLES START AT T%1\n").arg(initialHoleIndex);
    excellonHeader << QString("; THROUGH (PLATED) HOLES START AT T%1\n").arg(initialPlatedIndex);

    excellonHeader << "M48\n";
    excellonHeader << "INCH\n";
    dumpHoles(excellonHeader, excellonBody, nonPlatedHoles, initialHoleIndex, svgSize.height(), outline);
    dumpHoles(excellonHeader, excellonBody, platedHoles, initialPlatedIndex, svgSize.height(), outline);
    excellonBody << "T00\nM30\n";
    QFile f(exportDir + "/" + filename+ "_drill.txt");
    f.open(QIODevice::WriteOnly);
    QTextStream fs(&f);
    fs << excellonHeader.join("") << "%\n" << excellonBody.join("");
    f.close();
}


void GerberGenerator::displayMessage(const QString & message, bool displayMessageBoxes) {
	// don't use QMessageBox if running conversion as a service
	if (displayMessageBoxes) {
		QMessageBox::warning(NULL, QObject::tr("Fritzing"), message);
		return;

	}

	DebugDialog::debug(message);
}

QString GerberGenerator::cleanOutline(const QString & outlineSvg)
{
	QDomDocument doc;
	doc.setContent(outlineSvg);
	QList<QDomElement> leaves;
    QDomElement root = doc.documentElement();
    TextUtils::collectLeaves(root, leaves);
	QDomNodeList textNodes = root.elementsByTagName("text");
	for (int t = 0; t < textNodes.count(); t++) {
		leaves << textNodes.at(t).toElement();
	}

	if (leaves.count() == 0) return "";
	if (leaves.count() == 1) return outlineSvg;

	if (leaves.count() > 1) {
		for (int i = 0; i < leaves.count(); i++) {
			QDomElement leaf = leaves.at(i);
			if (leaf.attribute("id", "").compare(MagicBoardOutlineID) == 0) {
				for (int j = 0; j < leaves.count(); j++) {
					if (i != j) {
						QDomElement jleaf = leaves.at(j);
						jleaf.parentNode().removeChild(jleaf);
					}
				}

				return doc.toString();
			}
		}
	}

	if (leaves.count() == 0) return "";

	return outlineSvg;
}



void GerberGenerator::exportPickAndPlace(const QString & prefix, const QString & exportDir, ItemBase * board, PCBSketchWidget * sketchWidget, bool displayMessageBoxes)
{
    QPointF bottomLeft = board->sceneBoundingRect().bottomLeft();
    QSet<ItemBase *> itemBases;
    foreach (QGraphicsItem * item, sketchWidget->scene()->collidingItems(board)) {
        ItemBase * itemBase = dynamic_cast<ItemBase *>(item);
        if (itemBase == NULL) continue;
        if (itemBase == board) continue;
        if (itemBase->itemType() == ModelPart::Wire) continue;

        itemBase = itemBase->layerKinChief();
        if (!itemBase->isEverVisible()) continue;
        if (itemBase == board) continue;

        itemBases.insert(itemBase->layerKinChief());
    }

    QString outname = exportDir + "/" + prefix + "_pnp.txt";
    QFile out(outname);
	if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
		displayMessage(QObject::tr("Unable to save pick and place file: %2").arg(outname), displayMessageBoxes);
		return;
	}

    QTextStream stream(&out);
    stream << "*Pick And Place List\n"
        << "*Company=\n"
        << "*Author=\n"
        //*Tel=
        //*Fax=
        << "*eMail=\n"
        << "*\n"
        << QString("*Project=%1\n").arg(prefix)
        // *Variant=<alle Bauteile>
        << QString("*Date=%1\n").arg(QTime::currentTime().toString())
        << QString("*CreatedBy=Fritzing %1\n").arg(Version::versionString())
        << "*\n"
        << "*\n*Coordinates in mm, always center of component\n"
        << "*Origin 0/0=Lower left corner of PCB\n"
        << "*Rotation in degree (0-360, math. pos.)\n"
        << "*\n"
        << "*No;Value;Package;X;Y;Rotation;Side;Name\n"
        ;

    QStringList valueKeys;
    valueKeys << "resistance" << "capacitance" << "inductance" << "voltage"  << "current" << "power";

    int ix = 1;
    foreach (ItemBase * itemBase, itemBases) {
        QString value;
        foreach (QString valueKey, valueKeys) {
            value = itemBase->modelPart()->localProp(valueKey).toString();
            if (!value.isEmpty()) break;

            value = itemBase->modelPart()->properties().value(valueKey);
            if (!value.isEmpty()) break;
        }

        QPointF loc = itemBase->sceneBoundingRect().center();
        QTransform transform = itemBase->transform();
        // doesn't account for scaling
        double angle = atan2(transform.m12(), transform.m11()) * 180 / M_PI;
        // No;Value;Package;X;Y;Rotation;Side;Name
        QString string = QString("%1;%2;%3;%4;%5;%6;%7;%8\n")
            .arg(ix++)
            .arg(value)
            .arg(itemBase->modelPart()->properties().value("package"))
            .arg(GraphicsUtils::pixels2mm(loc.x() - bottomLeft.x(), GraphicsUtils::SVGDPI))
            .arg(GraphicsUtils::pixels2mm(loc.y() - bottomLeft.y(), GraphicsUtils::SVGDPI))
            .arg(angle)
            .arg(itemBase->viewLayerID() == ViewLayer::Copper1 ? "Top" : "Bottom")
            .arg(itemBase->instanceTitle())
        ;
        stream << string;
        stream.flush();
    }

    out.close();
}

QString GerberGenerator::renderTo(const LayerList &layers, ItemBase *board, PCBSketchWidget *sketchWidget) {
    RenderThing renderThing;
    renderThing.printerScale = GraphicsUtils::SVGDPI;
    renderThing.blackOnly = true;
    renderThing.dpi = GraphicsUtils::StandardFritzingDPI;
    renderThing.hideTerminalPoints = true;
    renderThing.selectedItems = renderThing.renderBlocker = false;
	QString svg = sketchWidget->renderToSVG(renderThing, board, layers);
    return svg;
}
