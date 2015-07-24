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

$Revision: 6976 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-21 09:50:09 +0200 (So, 21. Apr 2013) $

********************************************************************/

#include <QWheelEvent>
#include <QScrollBar>
#include <QSettings>
//#include <QGLWidget>

#include "zoomablegraphicsview.h"
#include "../utils/zoomslider.h"
#include "../utils/misc.h"

const int ZoomableGraphicsView::MinScaleValue = 5;
const int ZoomableGraphicsView::MaxScaleValue = 3000;

ZoomableGraphicsView::WheelMapping ZoomableGraphicsView::m_wheelMapping =
#ifdef Q_OS_WIN
    ZoomPrimary;
#else
    ScrollPrimary;
#endif

bool FirstTime = true;

ZoomableGraphicsView::ZoomableGraphicsView( QWidget * parent )
    : QGraphicsView(parent)
{
    m_viewFromBelow = false;
    m_scaleValue = 100;
    m_minScaleValue = MinScaleValue;
    m_maxScaleValue = MaxScaleValue;
    m_acceptWheelEvents = true;
    m_initialModifiers = 0;
    m_stuckModifierTimer = new QTimer(this);
    m_stuckModifierTimer->setSingleShot(true);
    connect(m_stuckModifierTimer, SIGNAL(timeout()), this, SLOT(unstickModifiers()));

    if (FirstTime) {
        FirstTime = false;
        QSettings settings;
        m_wheelMapping = (WheelMapping) settings.value("wheelMapping", m_wheelMapping).toInt();
        if (m_wheelMapping >= WheelMappingCount) {
            m_wheelMapping = ScrollPrimary;
        }
    }
    //setViewport(new QGLWidget);
}

void ZoomableGraphicsView::unstickModifiers() {
    m_initialModifiers = 0;
}

void ZoomableGraphicsView::wheelEvent(QWheelEvent* event) {
    if (!m_acceptWheelEvents) {
        QGraphicsView::wheelEvent(event);
        return;
    }

#ifdef Q_OS_MAC
    // On Mac OS, wheel events can have inertia, so we should only honor the
    // modifier key state at the beginning of the "scroll phase".
    if (event->phase() == Qt::ScrollBegin) {
        m_initialModifiers = event->modifiers();
    }
    else if (m_initialModifiers == 0) {
        m_initialModifiers = event->modifiers();
    }
    // Unfortunately, due to some issues in QT 5.2.x, the scroll phase can
    // go unreported or trigger incorrectly. In order to relieve "stuck" states,
    // we use a timer to "unstick" the modifier state. This should be fixed in
    // QT 5.5.x.
    if (m_stuckModifierTimer->isActive()) {
        m_stuckModifierTimer->stop();
    }
    // 100 msec wait after wheel events should suffice
    m_stuckModifierTimer->start(100);
#else
    m_initialModifiers = event->modifiers();
#endif

    bool control = m_initialModifiers & Qt::ControlModifier;
    bool alt = m_initialModifiers & altOrMetaModifier();
    bool shift = m_initialModifiers & Qt::ShiftModifier;

    if (shift) {
        QGraphicsView::wheelEvent(event);
        return;
    }

    bool doZoom = false;
    bool doHorizontal = false;
    bool doVertical = false;

    // doZoom = true;
    switch (m_wheelMapping) {
     case ScrollPrimary:
         if (control || alt) doZoom = true;
         else {
             if (event->orientation() == Qt::Horizontal) {
                 doHorizontal = true;
             }
             else {
                 doVertical = true;
             }
         }
         break;
     case ZoomPrimary:
         if (control || alt) {
             if (event->orientation() == Qt::Horizontal) {
                 doHorizontal = true;
             }
             else {
                 doVertical = true;
             }
         }
         else doZoom = true;
         break;
     default:
         // shouldn't happen
         return;
    }

    if (shift && (doVertical || doHorizontal)) {
        if (doVertical) {
            doVertical = false;
            doHorizontal = true;
        }
        else {
            doVertical = true;
            doHorizontal = false;
        }
    }

    int numSteps = event->delta() / 8;
    if (doZoom) {
        double delta = ((double) event->delta() / 120) * ZoomSlider::ZoomStep;
        if (delta == 0) return;

        // Scroll zooming relative to the current size
        relativeZoom(delta, event->pos());

        emit wheelSignal();
    }
    else if (doVertical) {
        verticalScrollBar()->setValue( verticalScrollBar()->value() - numSteps);
    }
    else if (doHorizontal) {
        horizontalScrollBar()->setValue( horizontalScrollBar()->value() - numSteps);
    }
}

void ZoomableGraphicsView::relativeZoom(double step, QPoint viewCenterPos) {
    double newScaleValue = m_scaleValue + step;

    // Prevent zooming out too much
    if (newScaleValue < m_minScaleValue) {
        m_scaleValue = m_minScaleValue;
        emit zoomOutOfRange(m_scaleValue);
        return;
    }

    // Prevent zooming in too much
    if (newScaleValue > m_maxScaleValue) {
        m_scaleValue = m_maxScaleValue;
        emit zoomOutOfRange(m_scaleValue);
        return;
    }

    // Zoom Factor
    QPointF oldPos;

    // We'll be doing the job that anchors are supposed to do, so use NoAnchor
    this->setTransformationAnchor(QGraphicsView::NoAnchor);
    this->setResizeAnchor(QGraphicsView::NoAnchor);

    if (!viewCenterPos.isNull()) {
        // Save the scene position prior to zooming
        oldPos = this->mapToScene(viewCenterPos);
    }

    // Zoom
    double zoomFactor = newScaleValue / m_scaleValue;
    this->scale(zoomFactor, zoomFactor);

    // Translate viewport back to the place we should be centered on (usually
    // the mouse), if the position is given.
    if (!viewCenterPos.isNull()) { 
        // Get the scene position after zooming
        QPointF newPos = this->mapToScene(viewCenterPos);
        QPointF delta = newPos - oldPos;

        if (this->horizontalScrollBar()->isVisible()) {
            // If scroll bars are visible, we just translate the viewport
            this->translate(delta.x(), delta.y());
        } else {
            // Otherwise, we need to resize the scene to keep its center on point
            QRectF sr = this->sceneRect();
            this->setSceneRect(
                sr.left() - delta.x(),
                sr.top() - delta.y(),
                sr.width(),
                sr.height());
        }
    }

    m_scaleValue = newScaleValue;
    emit zoomChanged(m_scaleValue);
}

void ZoomableGraphicsView::absoluteZoom(double percent) {
    relativeZoom(percent - m_scaleValue, QPoint());
}

double ZoomableGraphicsView::currentZoom() {
    return m_scaleValue;
}

void ZoomableGraphicsView::setAcceptWheelEvents(bool accept) {
    m_acceptWheelEvents = accept;
}

void ZoomableGraphicsView::setWheelMapping(WheelMapping wm) {
    m_wheelMapping = wm;
}

ZoomableGraphicsView::WheelMapping ZoomableGraphicsView::wheelMapping() {
    return m_wheelMapping;
}

bool ZoomableGraphicsView::viewFromBelow() {
    return m_viewFromBelow;
}

void ZoomableGraphicsView::setViewFromBelow(bool viewFromBelow) {
    if (m_viewFromBelow == viewFromBelow) return;

    m_viewFromBelow = viewFromBelow;

    this->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    QTransform transform;
    transform.scale(-1, 1);
    this->setTransform(transform, true);
}

