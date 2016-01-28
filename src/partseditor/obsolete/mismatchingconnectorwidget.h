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



#ifndef MISMATCHINGCONNECTORWIDGET_H_
#define MISMATCHINGCONNECTORWIDGET_H_

#include <QLabel>
#include "abstractconnectorinfowidget.h"
#include "../connectors/connector.h"

class MismatchingConnectorWidget : public AbstractConnectorInfoWidget {
	Q_OBJECT
	public:
		MismatchingConnectorWidget(class ConnectorsInfoWidget *topLevelContainer, ViewLayer::ViewIdentifier viewId, const QString &connId, QWidget *parent, bool isInView = true, Connector* conn = NULL);
		~MismatchingConnectorWidget();
		
		void setSelected(bool selected, bool doEmitChange=true);
		bool onlyMissingThisView(ViewLayer::ViewIdentifier viewId);
		void addViewPresence(ViewLayer::ViewIdentifier viewId);
		void removeViewPresence(ViewLayer::ViewIdentifier viewId);
		const QString &connId();
		Connector *prevConn();
		QList<ViewLayer::ViewIdentifier> views();
		QList<ViewLayer::ViewIdentifier> missingViews();
		bool presentInAllViews();

	signals:
		void completeConn(MismatchingConnectorWidget*);

	protected slots:
		void emitCompletion();

	protected:
		void mousePressEvent(QMouseEvent * event);
		QString viewsString();

		QList<ViewLayer::ViewIdentifier> m_missingViews;
		QString m_connId;
		QPointer<Connector> m_prevConn; // If this connector info used to be a not mismatching one, we save that info here

		QLabel *m_connIdLabel;
		QLabel *m_connMsgLabel;

		static QList<ViewLayer::ViewIdentifier> AllViews;
};

#endif /* MISMATCHINGCONNECTORWIDGET_H_ */
