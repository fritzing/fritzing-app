/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2014 Fachhochschule Potsdam - http://fh-potsdam.de

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


********************************************************************/

#ifndef CLIPPERHELPERS_H
#define CLIPPERHELPERS_H

#include <limits>
#include "../lib/clipper/clipper.hpp"
#include <QPaintEngine>
#include <fstream>

inline ClipperLib::JoinType qtToClipperJoinType(Qt::PenJoinStyle style) {
    switch (style) {
        case Qt::MiterJoin:
            return ClipperLib::jtMiter;
        case Qt::BevelJoin:
            return ClipperLib::jtSquare;
        case Qt::RoundJoin:
            return ClipperLib::jtRound;
        case Qt::SvgMiterJoin:
            return ClipperLib::jtMiter;
        default:
            return ClipperLib::jtRound;
    }
}

inline ClipperLib::EndType qtToClipperEndType(Qt::PenCapStyle style, bool open, bool fill) {
    if (open)
        switch (style) {
            case Qt::FlatCap:
                return ClipperLib::etOpenButt;
            case Qt::SquareCap:
                return ClipperLib::etOpenSquare;
            case Qt::RoundCap:
                return ClipperLib::etOpenRound;
            default:
                return ClipperLib::etOpenRound;
        }
    else if (fill)
        return ClipperLib::etClosedPolygon;
    else
        return ClipperLib::etClosedLine;
}

inline ClipperLib::PolyFillType qtToClipperFillType(QPaintEngine::PolygonDrawMode mode) {
    switch (mode) {
        case QPaintEngine::OddEvenMode:
            return ClipperLib::pftEvenOdd;
        case QPaintEngine::WindingMode:
            return ClipperLib::pftNonZero;
        case QPaintEngine::ConvexMode:
            return ClipperLib::pftNonZero;
        case QPaintEngine::PolylineMode:
            return ClipperLib::pftNonZero;
    }
    return ClipperLib::pftNonZero;
}

inline ClipperLib::Path pointsToClipper(const QPointF *points, int pointCount, QMatrix matrix = QMatrix()) {
    ClipperLib::Path clipperPath;
    for (int i = 0; i < pointCount; i++) {
        const QPointF p2 = matrix.map(points[i]);
        clipperPath << ClipperLib::IntPoint((ClipperLib::cInt) (p2.x()), (ClipperLib::cInt) (p2.y()));
    }
    return clipperPath;
}

inline ClipperLib::Path polygonToClipper(QPolygonF poly, const QMatrix &matrix = QMatrix()) {
    return pointsToClipper(poly.data(), poly.size(), matrix);
}

inline ClipperLib::Paths polygonsToClipper(QList<QPolygonF> polys, const QMatrix &matrix = QMatrix()) {
    ClipperLib::Paths paths;
    for (int i = 0; i < polys.size(); i++) {
        paths << polygonToClipper(polys[i], matrix);
    }
    return paths;
}

inline QRectF boundingBox(ClipperLib::Paths paths) {
    int minX = std::numeric_limits<int>::max();
    int minY = std::numeric_limits<int>::max();
    int maxX = std::numeric_limits<int>::min();
    int maxY = std::numeric_limits<int>::min();
    for (size_t i = 0; i < paths.size(); i++) {
        for (size_t j = 0; j < paths[i].size(); j++) {
            ClipperLib::IntPoint pt = paths[i][j];
            minX = std::min(minX, (int) pt.X);
            minY = std::min(minY, (int) pt.Y);
            maxX = std::max(maxX, (int) pt.X);
            maxY = std::max(maxY, (int) pt.Y);
        }
    }
    return QRectF(minX, minY, maxX - minX, maxY - minY);
}

inline QString clipperPathsToSVG(ClipperLib::Paths &paths, double clipperDPI, bool withSVGElement = true) {
    QRectF bounds = boundingBox(paths);
    QStringList pSvg;

    if (withSVGElement)
        pSvg << TextUtils::makeSVGHeader(1, clipperDPI, bounds.width() / clipperDPI, bounds.height() / clipperDPI);
    pSvg << QString("<path fill='black' stroke='none' stroke-width='0' d='");
    for (size_t i = 0; i < paths.size(); i++) {
        for (size_t j = 0; j < paths[i].size(); j++) {
            ClipperLib::IntPoint pt = paths[i][j];
            pSvg << QString(j == 0 ? "M" : "L");
            pSvg << QString("%1,%2 ").arg(pt.X - bounds.left()).arg(pt.Y - bounds.top());
        }
        pSvg << "Z";
    }
    pSvg << "'/>\n";
    if (withSVGElement)
        pSvg << "</svg>\n";
    return pSvg.join("");
}

inline void clipperPathsToSVGFile(ClipperLib::Paths &paths, double clipperDPI, QString fileName) {
    std::ofstream file(fileName.toStdString().c_str());
    file << clipperPathsToSVG(paths, clipperDPI).toStdString();
}

inline QString imageToSVGPath(QImage &image, double res) {
    ClipperLib::Paths vectorized;
    ClipperLib::Clipper cp;
    ClipperLib::Paths lineSpans;
    for (int j = 0; j < image.height(); j++) {
        bool previousPix = false;
        int startIndex = 0;
        for (int i = 0; i < image.width(); i++) {
            bool pix = qGray(image.pixel(i, j)) != 0;
            if (pix != previousPix || i == image.width() - 1) if (pix) {
                ClipperLib::Path span;
                span << ClipperLib::IntPoint(startIndex, j) << ClipperLib::IntPoint(i + 1, j)
                        << ClipperLib::IntPoint(i + 1, j + 1) << ClipperLib::IntPoint(startIndex, j + 1);
                lineSpans << span;
                startIndex = 0;
            } else {
                startIndex = i;
            }
            previousPix = pix;
        }
    }
    cp.AddPaths(lineSpans, ClipperLib::ptSubject, true);
    cp.AddPaths(vectorized, ClipperLib::ptClip, true);
    ClipperLib::Paths result;
    cp.Execute(ClipperLib::ctUnion, result, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
    return clipperPathsToSVG(result, res, false);
}

#endif // CLIPPERHELPERS_H
