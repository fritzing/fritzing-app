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


#ifndef PARTSEDITORVIEWSWIDGET_H_
#define PARTSEDITORVIEWSWIDGET_H_

#include <QWidget>
#include <QSplitter>

#include "partseditorview.h"
#include "connectorsinfowidget.h"

class PartsEditorViewsWidget : public QFrame {
Q_OBJECT
	public:
		PartsEditorViewsWidget(SketchModel *sketchModel, class WaitPushUndoStack *undoStack, ConnectorsInfoWidget* info, QWidget *parent, class ItemBase * fromItem);
		~PartsEditorViewsWidget();

		void copySvgFilesToDestiny(const QString &partFileName);
		void loadViewsImagesFromModel(PaletteModel *paletteModel, ModelPart *modelPart);
		const QDir& tempDir();
		void aboutToSave();
		void updatePinsInfo(QList< QPointer<ConnectorShared> > connsShared);
		QCheckBox *showTerminalPointsCheckBox();

		bool imagesLoadedInAllViews();
		void setViewItems(class ItemBase *, class ItemBase *, class ItemBase *);

		PartsEditorView *breadboardView();
		PartsEditorView *schematicView();
		PartsEditorView *pcbView();

		void connectTerminalRemoval(const ConnectorsInfoWidget* connsInfo);
		bool connectorsPosOrSizeChanged();

	public slots:
		void repaint();
		void drawConnector(Connector*);
		void drawConnector(ViewLayer::ViewIdentifier, Connector*);
		void removeConnectorFrom(const QString&,ViewLayer::ViewIdentifier);
		void showHideTerminalPoints(int checkState);
		void informConnectorSelection(const QString &connId);
		void setMismatching(ViewLayer::ViewIdentifier viewId, const QString &id, bool mismatching);

	signals:
		void connectorsFoundSignal(QList< QPointer<Connector> >);
		void connectorSelectedInView(const QString& connId);

	protected:
		PartsEditorView * createViewImageWidget(
			SketchModel* sketchModel, class WaitPushUndoStack *undoStack,
			ViewLayer::ViewIdentifier viewId, QString iconFileName, QString startText,
			ConnectorsInfoWidget* info, ViewLayer::ViewLayerID viewLayerId, class ItemBase * fromItem
		);
		void init();

		QWidget *addZoomControlsAndBrowseButton(PartsEditorView *view);

		bool showingTerminalPoints();
		bool checkStateToBool(int checkState);

		void connectPair(PartsEditorView *v1, PartsEditorView *v2);
		void connectToThis(PartsEditorView *v);

		QPointer<PartsEditorView> m_breadView;
		QPointer<PartsEditorView> m_schemView;
		QPointer<PartsEditorView> m_pcbView;
		QHash<ViewLayer::ViewIdentifier,PartsEditorView*> m_views;
		QSplitter *m_viewsContainter;

		QPointer<QCheckBox> m_showTerminalPointsCheckBox;
		QPointer<QLabel> m_guidelines;

		bool m_connsPosChanged;

	protected:
		static QString EmptyBreadViewText;
		static QString EmptySchemViewText;
		static QString EmptyPcbViewText;
};

#endif /* PARTSEDITORVIEWSWIDGET_H_ */
