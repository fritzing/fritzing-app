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

$Revision: 6904 $:
$Author: irascibl@gmail.com $:
$Date: 2013-02-26 16:26:03 +0100 (Di, 26. Feb 2013) $

********************************************************************/


#ifndef PARTSEDITORVIEW_H_
#define PARTSEDITORVIEW_H_

#include "../sketch/sketchwidget.h"
#include "../model/palettemodel.h"
#include "partseditorpaletteitem.h"
#include "partseditorconnectorspaletteitem.h"
#include "partseditorconnectorsconnectoritem.h"

#include <QList>
#include <QStringList>
#include <QComboBox>


struct ConnectorTerminalSvgIdPair {
	ConnectorTerminalSvgIdPair() {
	}
	QString connectorId;
	QString terminalId;
	QString connectorName;
};

class PartsEditorView : public SketchWidget {
	Q_OBJECT

	public:
		PartsEditorView(
			ViewLayer::ViewIdentifier, QDir tempDir,
			bool showingTerminalPoints, QGraphicsProxyWidget *startItem=0,
			QWidget *parent=0, int size=150, bool deleteModelPartOnClearScene=false,
			class ItemBase * fromItem = NULL);
		~PartsEditorView();

		// general
		QDir tempFolder();
		bool isEmpty();
		ViewLayer::ViewLayerID connectorsLayerId();
		QString terminalIdForConnector(const QString &connId);
		void addFixedToBottomRight(QWidget *widget);
		bool imageLoaded();

		// specs
		void loadSvgFile(ModelPart * modelPart);
		void copySvgFileToDestiny(const QString &partFileName);

		const QString svgFilePath();
		const SvgAndPartFilePath& svgFileSplit();

		// conns
		void drawConector(Connector *conn, bool showTerminalPoint);
		void removeConnector(const QString &connId);
		void inFileDefinedConnectorChanged(PartsEditorConnectorsConnectorItem *connItem);
		void aboutToSave(bool fakeDefaultIfNotIn);
		void updatePinsInfo(QList< QPointer<class ConnectorShared> > conns);

		void showTerminalPoints(bool show);
		bool showingTerminalPoints();

		QString svgIdForConnector(const QString &connId);
		PartsEditorConnectorsPaletteItem *myItem();

		bool connsPosOrSizeChanged();
		void setViewItem(ItemBase *);
        void setPaletteModel(PaletteModel *);

	public slots:
		// general
		void loadFromModel(PaletteModel *paletteModel, ModelPart * modelPart);
		void addItemInPartsEditor(ModelPart * modelPart, SvgAndPartFilePath * svgFilePath);

		// specs
		void loadFile();
		void loadSvgFile(const QString& origPath);
		void updateModelPart(const QString& origPath);

		// conns
		void informConnectorSelection(const QString& connId);
		void informConnectorSelectionFromView(const QString& connId);
		void setMismatching(ViewLayer::ViewIdentifier viewId, const QString &id, bool mismatching);
		void checkConnectorLayers(ViewLayer::ViewIdentifier, const QString & connId, Connector* existingConnector, Connector * newConnector);

	protected slots:
		void recoverTerminalPointsState();
		void fitCenterAndDeselect();
		void ensureFixedItemsPositions();

	signals:
		// conns
		void connectorsFoundSignal(ViewLayer::ViewIdentifier viewId, const QList< QPointer<Connector> > &conns);
		void svgFileLoadNeeded(const QString &filepath);
		void connectorSelected(const QString& connId);
		void removeTerminalPoint(const QString &connId, ViewLayer::ViewIdentifier vid);

	protected:
		// general
		PartsEditorPaletteItem *newPartsEditorPaletteItem(ModelPart * modelPart);
		PartsEditorPaletteItem *newPartsEditorPaletteItem(ModelPart * modelPart, SvgAndPartFilePath *path);

		void setDefaultBackground();
		void clearScene();
		void removeConnectors();
		void addDefaultLayers(class ItemBase * fromItem);

		void wheelEvent(QWheelEvent* event);
		void drawBackground(QPainter *painter, const QRectF &rect);

		ItemBase * addItemAux(ModelPart * modelPart, ViewLayer::ViewLayerSpec, const ViewGeometry & viewGeometry, long id, PaletteItem* paletteItem, bool doConnectors, ViewLayer::ViewIdentifier, bool temporary);

		ModelPart *createFakeModelPart(SvgAndPartFilePath *svgpath);
		ModelPart *createFakeModelPart(const QHash<QString,ConnectorTerminalSvgIdPair> &connIds, const QStringList &layers, const QString &svgFilePath);

		const QHash<QString,ConnectorTerminalSvgIdPair> getConnectorsSvgIds(const QString &path);
		void getConnectorsSvgIdsAux(QDomElement &docElem);
		const QStringList getLayers(const QString &path);
		const QStringList getLayers(const QDomDocument *dom, bool fakeDefaultIfNone=true);

		QString getOrCreateViewFolderInTemp();
		bool ensureFilePath(const QString &filePath);


		// TODO: retire all singular findConnectorsLayerId() for plural findConnectorsLayerIds()
		QStringList findConnectorsLayerIds(QDomDocument *svgDom);
		void findConnectorsLayerIdsAux(QStringList &result, QDomElement &docElem);
		void findConnectorsLayerId();
		QString findConnectorsLayerId(QDomDocument *svgDom);
		bool findConnectorsLayerIdAux(QString &result, QDomElement &docElem, QStringList &prevLayers);
		bool terminalIdForConnectorIdAux(QString &result, const QString &connId, QDomElement &docElem, bool wantTerminal);
		QString getLayerFileName(ModelPart * modelPart);
		LayerList defaultLayers();
		QStringList defaultLayerAsStringlist();


		// SVG fixing
		void beforeSVGLoading(const QString &filename, bool &canceled);
		bool fixFonts(QString &fileContent, const QString &filename, bool &canceled);
		bool removeFontFamilySingleQuotes(QString &fileContent, const QString &filename);
		bool fixUnavailableFontFamilies(QString &fileContent, const QString &filename, bool &canceled);
		QSet<QString> getAttrFontFamilies(const QString &fileContent);
		QSet<QString> getFontFamiliesInsideStyleTag(const QString &fileContent);


		// specs
		void setSvgFilePath(const QString &filePath);
		void copyToTempAndRenameIfNecessary(SvgAndPartFilePath *filePathOrig);
		QString createSvgFromImage(const QString &filePath);

		QString setFriendlierSvgFileName(const QString &partFileName);


		// conns
		void mousePressEvent(QMouseEvent *event);
		void mouseMoveEvent(QMouseEvent *event);
		void mouseReleaseEvent(QMouseEvent *event);
		void resizeEvent(QResizeEvent * event);
		void connectItem();
		void createConnector(Connector *conn, const QSize &connSize, bool showTerminalPoint);
		void setItemProperties();
		bool isSupposedToBeRemoved(const QString& id);

		bool addConnectorsIfNeeded(QDomDocument *svgDom, const QSizeF &sceneViewBox, const QRectF &svgViewBox, const QString &connectorsLayerId);
		bool removeConnectorsIfNeeded(QDomElement &docEle);
		bool updateTerminalPoints(QDomDocument *svgDom, const QSizeF &sceneViewBox, const QRectF &svgViewBox, const QString &connectorsLayerId);
		bool addDefaultLayerIfNotInSvg(QDomDocument *svgDom, bool fakeDefaultIfNone);
		QString svgIdForConnector(Connector* conn, const QString &connId);

		void updateSvgIdLayer(const QString &connId, const QString &terminalId, const QString &connectorsLayerId);
		void removeTerminalPoints(const QStringList &tpIdsToRemove, QDomElement &docElem);
		void addNewTerminalPoints(
				const QList<PartsEditorConnectorsConnectorItem*> &connsWithNewTPs, QDomDocument *svgDom,
				const QSizeF &sceneViewBox, const QRectF &svgViewBox, const QString &connectorsLayerId
		);
		QRectF mapFromSceneToSvg(const QRectF &sceneRect, const QSizeF &defaultSize, const QRectF &viewBox);
		bool addRectToSvg(QDomDocument* svgDom, const QString &id, const QRectF &rect, const QString &connectorsLayerId);
		bool addRectToSvgAux(QDomElement &docElem, const QString &connectorsLayerId, QDomElement &rectElem);
		QString saveSvg(const QString & svg, const QString & newFilePath);

		void addFixedToTopLeftItem(QGraphicsItem *item);
		void addFixedToTopRightItem(QGraphicsItem *item);
		void addFixedToBottomLeftItem(QGraphicsItem *item);
		void addFixedToCenterItem(QGraphicsItem *item);
		void addFixedToBottomRightItem(QGraphicsItem *item);

		void ensureFixedToTopLeftItems();
		void ensureFixedToTopRightItems();
		void ensureFixedToBottomLeftItems();
		void ensureFixedToBottomRightItems();
		void ensureFixedToCenterItems();

		void ensureFixedToTopLeft(QGraphicsItem* item);
		void ensureFixedToTopRight(QGraphicsItem* item);
		void ensureFixedToBottomLeft(QGraphicsItem* item);
		void ensureFixedToBottomRight(QGraphicsItem* item);
		void ensureFixedToCenter(QGraphicsItem* item);

		void clearFixedItems();
		void removeIfFixedPos(QGraphicsItem *item);
		double fixedItemWidth(QGraphicsItem* item);
		double fixedItemHeight(QGraphicsItem* item);
		void deleteItem(ItemBase *, bool deleteModelPart, bool doEmit, bool later);


protected:
		QPointer<PartsEditorPaletteItem> m_item; // just one item per view
		QDir m_tempFolder;
		bool m_deleteModelPartOnSceneClear;
		QPointer<QGraphicsProxyWidget> m_startItem;

		SvgAndPartFilePath *m_svgFilePath;
		QString m_originalSvgFilePath;

		QHash<QString /*id*/,PartsEditorConnectorsConnectorItem*> m_drawnConns;
		QStringList m_removedConnIds;

		QHash<QString/*connectorId*/,ConnectorTerminalSvgIdPair> m_svgIds;
		ViewLayer::ViewLayerID m_connsLayerID;
		bool m_svgLoaded;

		QString m_lastSelectedConnId;
		bool m_showingTerminalPoints;
		bool m_showingTerminalPointsBackup;
		QTimer *m_terminalPointsTimer;

		QList<QWidget* > m_fixedWidgets;
		ItemBase * m_viewItem;
		QTimer *m_fitItemInViewTimer;
		QList<QGraphicsItem*> m_fixedToTopLeftItems;
		QList<QGraphicsItem*> m_fixedToTopRightItems;
		QList<QGraphicsItem*> m_fixedToBottomLeftItems;
		QList<QGraphicsItem*> m_fixedToBottomRightItems;
		QList<QGraphicsItem*> m_fixedToCenterItems;
        PaletteModel * m_paletteModel;

	protected:
		static int ConnDefaultWidth;
		static int ConnDefaultHeight;
};


#endif /* PARTSEDITORVIEW_H_ */
