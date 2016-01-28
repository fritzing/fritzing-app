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

$Revision: 6912 $:
$Author: irascibl@gmail.com $:
$Date: 2013-03-09 08:18:59 +0100 (Sa, 09. Mrz 2013) $

********************************************************************/


#include "pecommands.h"
#include "pemainwindow.h"
#include "../debugdialog.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PEBaseCommand::PEBaseCommand(PEMainWindow * peMainWindow, QUndoCommand *parent)
	: BaseCommand(BaseCommand::CrossView, NULL, parent)
{
	m_peMainWindow = peMainWindow;
}

PEBaseCommand::~PEBaseCommand()
{
}

QString PEBaseCommand::getParamString() const {
    return "";
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ChangeMetadataCommand::ChangeMetadataCommand(PEMainWindow * peMainWindow, const QString & name, const QString  & oldValue, const QString & newValue, QUndoCommand *parent)
    : PEBaseCommand(peMainWindow, parent)
{
 	m_name = name;
	m_oldValue = oldValue;
	m_newValue = newValue;
}

void ChangeMetadataCommand::undo()
{
    m_peMainWindow->changeMetadata(m_name, m_oldValue, true);
}

void ChangeMetadataCommand::redo()
{
    if (m_skipFirstRedo) {
        m_skipFirstRedo = false;
    }
    else {
        m_peMainWindow->changeMetadata(m_name, m_newValue, true);
    }
}

QString ChangeMetadataCommand::getParamString() const {
	return "ChangeMetadataCommand " + 
        QString(" name:%1, old:%2, new:%3")
            .arg(m_name)
            .arg(m_oldValue)
            .arg(m_newValue)
        ;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ChangeTagsCommand::ChangeTagsCommand(PEMainWindow * peMainWindow, const QStringList  & oldTags, const QStringList & newTags, QUndoCommand *parent)
    : PEBaseCommand(peMainWindow, parent)
{
	m_old = oldTags;
	m_new = newTags;
}

void ChangeTagsCommand::undo()
{
    m_peMainWindow->changeTags(m_old, true);
}

void ChangeTagsCommand::redo()
{
    if (m_skipFirstRedo) {
        m_skipFirstRedo = false;
    }
    else {
        m_peMainWindow->changeTags(m_new, true);
    }
}

QString ChangeTagsCommand::getParamString() const {
	return "ChangeTagsCommand " + 
        QString(" old:%1, new:%2")
            .arg(m_old.count())
            .arg(m_new.count())
        ;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ChangePropertiesCommand::ChangePropertiesCommand(PEMainWindow * peMainWindow, const QHash<QString, QString> & oldProps, const QHash<QString, QString> & newProps, QUndoCommand *parent)
    : PEBaseCommand(peMainWindow, parent)
{
	m_old = oldProps;
	m_new = newProps;
}

void ChangePropertiesCommand::undo()
{
    m_peMainWindow->changeProperties(m_old, true);
}

void ChangePropertiesCommand::redo()
{
    if (m_skipFirstRedo) {
        m_skipFirstRedo = false;
    }
    else {
        m_peMainWindow->changeProperties(m_new, true);
    }
}

QString ChangePropertiesCommand::getParamString() const {
	return "ChangePropertiesCommand " + 
        QString(" old:%1, new:%2")
            .arg(m_old.count())
            .arg(m_new.count())
        ;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ChangeConnectorMetadataCommand::ChangeConnectorMetadataCommand(PEMainWindow * peMainWindow, ConnectorMetadata  * oldValue, ConnectorMetadata * newValue, QUndoCommand *parent)
    : PEBaseCommand(peMainWindow, parent)
{
	m_oldcm = *oldValue;
	m_newcm = *newValue;
}

void ChangeConnectorMetadataCommand::undo()
{
    m_peMainWindow->changeConnectorMetadata(&m_oldcm, true);
}

void ChangeConnectorMetadataCommand::redo()
{
    if (m_skipFirstRedo) {
        m_skipFirstRedo = false;
    }
    else {
        m_peMainWindow->changeConnectorMetadata(&m_newcm, true);
    }
}

QString ChangeConnectorMetadataCommand::getParamString() const {
	return "ChangeConnectorMetadataCommand " + 
        QString(" name:%1, old:%2, new:%3")
            .arg(m_oldcm.connectorName)
            .arg(m_newcm.connectorName)
        ;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ChangeFzpCommand::ChangeFzpCommand(PEMainWindow * peMainWindow, const QString & oldFzpFile, const QString & newFzpFile, QUndoCommand *parent)
    : PEBaseCommand(peMainWindow, parent)
{
	m_oldFzpFile = oldFzpFile;
    m_newFzpFile = newFzpFile;
}

void ChangeFzpCommand::undo()
{
	if (!m_redoOnly) {
		m_peMainWindow->restoreFzp(m_oldFzpFile);
	}
}

void ChangeFzpCommand::redo()
{
	if (!m_undoOnly) {
		m_peMainWindow->restoreFzp(m_newFzpFile);
	}
}

QString ChangeFzpCommand::getParamString() const {
	return "ChangeFzpCommand " + 
        QString(" old:%1 new:%2")
            .arg(m_oldFzpFile)
            .arg(m_newFzpFile)
        ;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ChangeSvgCommand::ChangeSvgCommand(PEMainWindow * peMainWindow, SketchWidget * sketchWidget, const QString  & oldFilename, const QString & newFilename, QUndoCommand *parent)
    : PEBaseCommand(peMainWindow, parent)
{
 	m_sketchWidget = sketchWidget;
	m_oldFilename = oldFilename;
	m_newFilename = newFilename;

}

void ChangeSvgCommand::undo()
{
    m_peMainWindow->changeSvg(m_sketchWidget, m_oldFilename, -1);
}

void ChangeSvgCommand::redo()
{
    m_peMainWindow->changeSvg(m_sketchWidget, m_newFilename, 1);
}

QString ChangeSvgCommand::getParamString() const {
	return "ChangeSvgCommand " + 
        QString(" id:%1, old:%2, new:%3")
            .arg(m_sketchWidget->viewID())
            .arg(m_oldFilename)
            .arg(m_newFilename)
        ;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RelocateConnectorSvgCommand::RelocateConnectorSvgCommand(PEMainWindow * peMainWindow, SketchWidget * sketchWidget, const QString  & id, const QString & terminalID, 
        const QString & oldGorn, const QString & oldGornTerminal, const QString & newGorn, const QString & newGornTerminal, 
        QUndoCommand *parent)
    : PEBaseCommand(peMainWindow, parent)
{
 	m_sketchWidget = sketchWidget;
	m_id = id;
	m_terminalID = terminalID;
	m_oldGorn = oldGorn;
	m_oldGornTerminal = oldGornTerminal;
	m_newGorn = newGorn;
	m_newGornTerminal = newGornTerminal;
}

void RelocateConnectorSvgCommand::undo()
{
    m_peMainWindow->relocateConnectorSvg(m_sketchWidget, m_id, m_terminalID, m_newGorn, m_newGornTerminal, m_oldGorn, m_oldGornTerminal, -1);
}

void RelocateConnectorSvgCommand::redo()
{
    m_peMainWindow->relocateConnectorSvg(m_sketchWidget, m_id, m_terminalID, m_oldGorn, m_oldGornTerminal, m_newGorn, m_newGornTerminal, 1);
}

QString RelocateConnectorSvgCommand::getParamString() const {
	return "RelocateConnectorSvgCommand " + 
        QString(" vid:%1 id:%2, terminalid:%3, oldgorn:%4, oldgornterminal:%5, newgorn:%6, newgornterminal:%7")
            .arg(m_sketchWidget->viewID())
            .arg(m_id)
            .arg(m_terminalID)
            .arg(m_oldGorn)
            .arg(m_oldGornTerminal)
            .arg(m_newGorn)
            .arg(m_newGornTerminal)
        ;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MoveTerminalPointCommand::MoveTerminalPointCommand(PEMainWindow * peMainWindow, SketchWidget * sketchWidget, const QString  & id, 
                QSizeF size, QPointF oldLocation, QPointF newLocation, QUndoCommand *parent)
    : PEBaseCommand(peMainWindow, parent)
{
 	m_sketchWidget = sketchWidget;
	m_id = id;
    m_size = size;
	m_oldLocation = oldLocation;
	m_newLocation = newLocation;
}

void MoveTerminalPointCommand::undo()
{
    m_peMainWindow->moveTerminalPoint(m_sketchWidget, m_id, m_size, m_oldLocation, -1);
}

void MoveTerminalPointCommand::redo()
{
    m_peMainWindow->moveTerminalPoint(m_sketchWidget, m_id, m_size, m_newLocation, 1);
}

QString MoveTerminalPointCommand::getParamString() const {
	return "RelocateConnectorSvgCommand " + 
        QString(" vid:%1, id:%2, old:%3,%4, new:%5,%6")
            .arg(m_sketchWidget->viewID())
            .arg(m_id)
            .arg(m_oldLocation.x())
            .arg(m_oldLocation.y())
            .arg(m_newLocation.x())
            .arg(m_newLocation.y())
        ;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RemoveBusConnectorCommand::RemoveBusConnectorCommand(PEMainWindow * peMainWindow, const QString & busID, const QString  & connectorID, bool inverted, QUndoCommand *parent)
    : PEBaseCommand(peMainWindow, parent)
{
 	m_busID = busID;
	m_connectorID = connectorID;
	m_inverted = inverted;
}

void RemoveBusConnectorCommand::undo()
{
	if (m_inverted) m_peMainWindow->removeBusConnector(m_busID, m_connectorID, true);
    else m_peMainWindow->addBusConnector(m_busID, m_connectorID);
}

void RemoveBusConnectorCommand::redo()
{
	if (m_inverted) m_peMainWindow->addBusConnector(m_busID, m_connectorID);
    else m_peMainWindow->removeBusConnector(m_busID, m_connectorID, true);
}

QString RemoveBusConnectorCommand::getParamString() const {
	return "RemoveBusConnectorCommand " + 
        QString(" busID:%1, connectorID:%2 inv:%3")
            .arg(m_busID)
            .arg(m_connectorID)
			.arg(m_inverted);
        ;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ChangeSMDCommand::ChangeSMDCommand(PEMainWindow * peMainWindow, const QString & before, const QString & after, 
									const QString  & oldFilename, const QString & newFilename, 
									QUndoCommand *parent)
    : PEBaseCommand(peMainWindow, parent)
{
 	m_before = before;
	m_after = after;
	m_oldFilename = oldFilename;
	m_newFilename = newFilename;
}

void ChangeSMDCommand::undo()
{
    m_peMainWindow->changeSMD(m_before, m_oldFilename, -1);
}

void ChangeSMDCommand::redo()
{
    m_peMainWindow->changeSMD(m_after, m_newFilename, 1);
}

QString ChangeSMDCommand::getParamString() const {
	return "ChangeSMDCommand " + 
        QString(" before:%1, after:%2 oldpath:%3, newPath:%4, oldorig:%5, neworig:%6")
            .arg(m_before)
            .arg(m_after)
            .arg(m_oldFilename)
            .arg(m_newFilename)
        ;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
