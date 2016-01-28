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

#include <QTreeWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileInfoList>
#include <QXmlStreamReader>
#include <QLabel>
#include <QDateTime>
#include <QHeaderView>

#include "recoverydialog.h"
#include "../utils/folderutils.h"
#include "../debugdialog.h"

CenteredTreeWidget::CenteredTreeWidget(QWidget * parent) : QTreeWidget(parent) {
	// no luck getting this styling to work from fritzing.qss
	setStyleSheet("::item { margin-left: 8px; margin-right: 8px; margin-top: 1px; margin-bottom: 1px; }");
}

QStyleOptionViewItem CenteredTreeWidget::viewOptions() const {
	// alignment not possible from a stylesheet
	QStyleOptionViewItem styleOptionViewItem = QTreeWidget::viewOptions();
	styleOptionViewItem.displayAlignment = Qt::AlignCenter;
	return styleOptionViewItem;
}

RecoveryDialog::RecoveryDialog(QFileInfoList fileInfoList, QWidget *parent, Qt::WindowFlags flags) : QDialog(parent, flags)
{
    m_recoveryList = new CenteredTreeWidget(this);
    m_recoveryList->setColumnCount(3);
    m_recoveryList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_recoveryList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_recoveryList->setSortingEnabled(false);
    m_recoveryList->setDragEnabled(false);
    m_recoveryList->setUniformRowHeights(true); // Small optimization
    m_recoveryList->setIconSize(QSize(0,0));
    m_recoveryList->setRootIsDecorated(false);

    QStringList headerLabels;
    headerLabels << tr("File") << tr("Last backup") << tr("Last saved");
    m_recoveryList->setHeaderLabels(headerLabels);
    m_recoveryList->header()->setDefaultAlignment(Qt::AlignCenter);
    m_recoveryList->header()->setDragEnabled(false);
    m_recoveryList->header()->setDragDropMode(QAbstractItemView::NoDragDrop);
    
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    m_recoveryList->header()->setResizeMode(QHeaderView::ResizeToContents);
    m_recoveryList->header()->setMovable(false);
#else
    m_recoveryList->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_recoveryList->header()->setSectionsMovable(false);
#endif

    connect(m_recoveryList, SIGNAL(itemSelectionChanged()), this, SLOT(updateRecoverButton()));

    QTreeWidgetItem *item;
    foreach (QFileInfo fileInfo, fileInfoList) {
        item = new QTreeWidgetItem;
        QString originalFileName = getOriginalFileName(fileInfo.absoluteFilePath());
        DebugDialog::debug(QString("Creating option for recoveryDialog file %1").arg(originalFileName));
        QFileInfo originalFileInfo(originalFileName);
        item->setData(0, Qt::UserRole, fileInfo.absoluteFilePath());
        if (originalFileInfo.exists()) {
            item->setText(0, originalFileInfo.fileName());
            item->setToolTip(0, originalFileInfo.absoluteFilePath());
            item->setData(1, Qt::UserRole, originalFileInfo.absoluteFilePath());
            item->setText(2, originalFileInfo.lastModified().toString());
        }
        else {
            item->setText(0, originalFileName);
            item->setText(2, tr("file not saved"));
            item->setData(1, Qt::UserRole, originalFileName);
        }
        item->setText(1, fileInfo.lastModified().toString());
        DebugDialog::debug(QString("Displaying recoveryDialog text %1 and data %2").arg(item->text(0)).arg(item->data(0,Qt::UserRole).value<QString>()));
        m_recoveryList->addTopLevelItem(item);
        m_fileList << item;
    }

    QVBoxLayout *layout = new QVBoxLayout();

	QLabel * label = new QLabel;
	label->setWordWrap(true);
	label->setText(tr("<p><b>Fritzing may have crashed, but some of the changes to the following files may be recovered.</b></p>"
					  "<p>The date and time each file was backed-up is displayed. "
					  "If the file was saved, that date and time is also listed for comparison.</p>"
					  "<p>The original files are still on your disk, if they were ever saved. "
					  "You can choose whether to overwrite the original file after you load its recovery file.</p>"
					  "<p><b>Select any files you want to recover from the list below.</b></p>"
					  ));

	layout->addWidget(label);
    layout->addWidget(m_recoveryList);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    layout->addLayout(buttonLayout);

    m_recover = new QPushButton(tr("&Recover"));
    m_recover->setDefault(true);
    m_recover->setEnabled(false);
    m_recover->setMaximumWidth(130);
    connect(m_recover, SIGNAL(clicked()), this, SLOT(accept()));

    m_ignore = new QPushButton(tr("&Ignore"));
    m_ignore->setMaximumWidth(130);
    connect(m_ignore, SIGNAL(clicked()), this, SLOT(reject()));

	buttonLayout->addSpacerItem(new QSpacerItem(0,0, QSizePolicy::Expanding));
    buttonLayout->addWidget(m_recover);
    buttonLayout->addWidget(m_ignore);
	buttonLayout->addSpacerItem(new QSpacerItem(0,0, QSizePolicy::Expanding));

    setLayout(layout);
	layout->setSizeConstraint(QLayout::SetMinAndMaxSize);
}

 QList<QTreeWidgetItem*> RecoveryDialog::getFileList() {
    return m_fileList;
}

 QString RecoveryDialog::getOriginalFileName(const QString & path)
 {
    QString originalName;
    QFile file(path);
	if (!file.open(QFile::ReadOnly)) {
		// TODO: not sure how else to handle this...
		DebugDialog::debug(QString("unable to open recovery file %1").arg(path));
		return originalName;
	}

    QXmlStreamReader xml(&file);
    xml.setNamespaceProcessing(false);
    while (!xml.atEnd()) {
        if (xml.readNext() == QXmlStreamReader::StartElement) {
            QString name = xml.name().toString();
            if (name == "originalFileName") {
                originalName = xml.readElementText();
                break;
            }
        }
    }
    file.close();
    return originalName;
 }

 void RecoveryDialog::updateRecoverButton() {
     if (m_recoveryList->selectedItems().isEmpty()) {
            m_recover->setEnabled(false);
     }
     else {
         m_recover->setEnabled(true);
     }
 }
