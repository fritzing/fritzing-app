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



#ifndef SINGLECONNECTORINFOWIDGET_H_
#define SINGLECONNECTORINFOWIDGET_H_

#include "../connectors/connector.h"
#include "abstractconnectorinfowidget.h"
#include "editablelinewidget.h"
#include "editabletextwidget.h"
#include "mismatchingconnectorwidget.h"

class ConnectorTypeWidget : public QFrame {
	Q_OBJECT
	public:
		ConnectorTypeWidget(Connector::ConnectorType type = Connector::Female, QWidget *parent=0);
		Connector::ConnectorType type();
		const QString &typeAsStr();
		void setType(Connector::ConnectorType type);
		void setSelected(bool selected);
		void setInEditionMode(bool edition);
		void cancel();

	protected slots:
		void toggleValue();

	protected:
		void setText(const QString &text);
		const QString &text();

		QLabel *m_noEditionModeWidget;
		QPushButton *m_editionModeWidget;

		QString m_text;
		bool m_isSelected;
		volatile bool m_isInEditionMode;
		Connector::ConnectorType m_typeBackUp;
};

class SingleConnectorInfoWidget : public AbstractConnectorInfoWidget {
	Q_OBJECT
	public:
		SingleConnectorInfoWidget(ConnectorsInfoWidget *topLevelContainer, WaitPushUndoStack *undoStack, Connector* connector=0, QWidget *parent=0);
		void setSelected(bool selected, bool doEmitChange=true);
		void setInEditionMode(bool inEditionMode);
		bool isInEditionMode();
		QSize sizeHint() const;
		QSize minimumSizeHint() const;
		QSize maximumSizeHint() const;

		Connector * connector();

		QString id();
		QString name();
		void setName(const QString &);
		QString description();
		QString type();
		Connector::ConnectorType connectorType();
		void setConnectorType(Connector::ConnectorType);

		MismatchingConnectorWidget *toMismatching(ViewLayer::ViewIdentifier viewId);

	protected slots:
		void editionCompleted();
		void editionCanceled();

	signals:
		void editionStarted();
		void editionFinished();
		//void connectorSelected(const QString&);

	protected:
		void toStandardMode();
		void toEditionMode();
		void startEdition();
		void createInputs();

		void mousePressEvent(QMouseEvent * event);

		void hideIfNeeded(QWidget* w);

		//QFrame *noEditFrame;
		QLabel *m_nameLabel;
		QLabel *m_nameDescSeparator;
		QLabel *m_descLabel;
		ConnectorTypeWidget *m_type;

		QFrame *m_nameEditContainer;
		QLineEdit *m_nameEdit;
		QFrame *m_descEditContainer;
		QTextEdit *m_descEdit;

		QPushButton *m_acceptButton;
		QPushButton *m_cancelButton;

		WaitPushUndoStack *m_undoStack;
		QPointer<Connector> m_connector;

		QString m_id;

		volatile bool m_isInEditionMode;
};

#endif /* SINGLECONNECTORINFOWIDGET_H_ */
