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

$Revision: 6984 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-22 23:44:56 +0200 (Mo, 22. Apr 2013) $

********************************************************************/

#ifndef RESIZABLEBOARD_H
#define RESIZABLEBOARD_H

#include <QRectF>
#include <QPainterPath>
#include <QPixmap>
#include <QVariant>
#include <QLineEdit>
#include <QCursor>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>

#include "paletteitem.h"

class Board : public PaletteItem 
{
	Q_OBJECT

public:
	// after calling this constructor if you want to render the loaded svg (either from model or from file), MUST call <renderImage>
	Board(ModelPart *, ViewLayer::ViewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel);
	~Board();

	QStringList collectValues(const QString & family, const QString & prop, QString & value);
	void paintHover(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	bool rotation45Allowed();
	bool stickyEnabled();
	PluralType isPlural();
	bool canFindConnectorsUnder();
	bool collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide);

protected:
    void setupLoadImage(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget);
	void setFileNameItems();
	virtual QStringList & getImageNames();
	virtual QStringList & getNewImageNames();
    virtual bool checkImage(const QString & filename);
	void unableToLoad(const QString & fileName, const QString & reason);
	bool canLoad(const QString & fileName, const QString & reason);
	virtual void prepLoadImageAux(const QString & fileName, bool addName);
    ViewLayer::ViewID useViewIDForPixmap(ViewLayer::ViewID, bool swappingEnabled);

protected:
    static void moreCheckImage(const QString & filename);
    static QString setBoardOutline(const QString & svg);

public:
	static QString OneLayerTranslated;
	static QString TwoLayersTranslated;

public:
    static bool isBoard(ItemBase *);
    static bool isBoard(ModelPart *);
    static QString convertToXmlName(const QString &);
    static QString convertFromXmlName(const QString &);

protected slots:
	void prepLoadImage();
	void fileNameEntry(const QString & filename);

protected:
    QPointer<QComboBox> m_fileNameComboBox;
    bool m_svgOnly;

protected:
    static QStringList BoardImageNames;
    static QStringList NewBoardImageNames;
};

class ResizableBoard : public Board 
{
	Q_OBJECT

public:
	// after calling this constructor if you want to render the loaded svg (either from model or from file), MUST call <renderImage>
	ResizableBoard(ModelPart *, ViewLayer::ViewID, const ViewGeometry & viewGeometry, long id, QMenu * itemMenu, bool doLabel);
	~ResizableBoard();

	virtual bool resizeMM(double w, double h, const LayerHash & viewLayers);
	void resizePixels(double w, double h, const LayerHash & viewLayers);
 	void loadLayerKin(const LayerHash & viewLayers, ViewLayer::ViewLayerPlacement);
	virtual void setInitialSize();
	QString retrieveSvg(ViewLayer::ViewLayerID, QHash<QString, QString> & svgHash, bool blackOnly, double dpi, double & factor);
	bool collectExtraInfo(QWidget * parent, const QString & family, const QString & prop, const QString & value, bool swappingEnabled, QString & returnProp, QString & returnValue, QWidget * & returnWidget, bool & hide);
	void saveParams();
	void getParams(QPointF &, QSizeF &);
	bool hasCustomSVG();
	QSizeF getSizeMM();
	void addedToScene(bool temporary);
	bool hasPartNumberProperty();
	void paintSelected(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	bool inResize();
    void figureHover();

public slots:
	virtual void widthEntry();
	virtual void heightEntry();
	void keepAspectRatio(bool checkState);
	void revertSize(bool);
    virtual void paperSizeChanged(int);

protected:
	enum Corner {
		NO_CORNER = 0,
		TOP_LEFT ,
		TOP_RIGHT,
		BOTTOM_LEFT,
		BOTTOM_RIGHT
	};

protected:
	void mousePressEvent(QGraphicsSceneMouseEvent * event );
	void mouseMoveEvent(QGraphicsSceneMouseEvent * event );
	void mouseReleaseEvent(QGraphicsSceneMouseEvent * event );
	void hoverEnterEvent(QGraphicsSceneHoverEvent * event );
	void hoverLeaveEvent(QGraphicsSceneHoverEvent * event );
	void hoverMoveEvent(QGraphicsSceneHoverEvent * event );
	QString makeBoardSvg(double mmW, double mmH, double milsW, double milsH);
	QString makeSilkscreenSvg(ViewLayer::ViewLayerID, double mmW, double mmH, double milsW, double milsH);
    QString makeSvg(double mmW, double mmH, const QString & layerTemplate);
	QStringList collectValues(const QString & family, const QString & prop, QString & value);
	virtual void loadTemplates();
	virtual double minWidth();
	virtual double minHeight();
	virtual ViewLayer::ViewID theViewID();
	virtual QString makeLayerSvg(ViewLayer::ViewLayerID viewLayerID, double mmW, double mmH, double milsW, double milsH);
	virtual QString makeFirstLayerSvg(double mmW, double mmH, double milsW, double milsH);
	virtual QString makeNextLayerSvg(ViewLayer::ViewLayerID, double mmW, double mmH, double milsW, double milsH);
	virtual void resizeMMAux(double w, double h);
	virtual ResizableBoard::Corner findCorner(QPointF p, Qt::KeyboardModifiers);
	void setKinCursor(QCursor &);
	void setKinCursor(Qt::CursorShape);
	QFrame * setUpDimEntry(bool includeAspectRatio, bool includeRevert, bool includePaperSizes, QWidget * & returnWidget);
	void fixWH();
	void setWidthAndHeight(double w, double h);
    QString getShapeForRenderer(const QString & svg, ViewLayer::ViewLayerID viewLayerID);
    void initPaperSizes();
    void updatePaperSizes(double width, double height);

protected:
	static const double CornerHandleSize;

	Corner m_corner;
	QSizeF m_boardSize;
	QPointF m_boardPos;
	QPointer<QLineEdit> m_widthEditor;
	QPointer<QLineEdit> m_heightEditor;
	bool m_keepAspectRatio;
	QSizeF m_aspectRatio;
	double m_currentScale;
	QPointer<QCheckBox> m_aspectRatioCheck;
    QPointer<QLabel> m_aspectRatioLabel;
    QPointer<QPushButton> m_revertButton;
    QPointer<QComboBox> m_paperSizeComboBox;

	QPointF m_resizeMousePos;
	QSizeF m_resizeStartSize;
	QPointF m_resizeStartPos;
	QPointF m_resizeStartTopLeft;
	QPointF m_resizeStartBottomRight;
	QPointF m_resizeStartTopRight;
	QPointF m_resizeStartBottomLeft;
	int m_decimalsAfter;

};

#endif
