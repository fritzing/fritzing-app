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



#ifndef PARTSBINCOMMANDS_H
#define PARTSBINCOMMANDS_H

#include <QUndoCommand>
#include <QHash>


class PartsBinBaseCommand : public QUndoCommand
{
public:
	PartsBinBaseCommand(class PartsBinPaletteWidget*, QUndoCommand* parent = 0);
	class PartsBinPaletteWidget* bin();

protected:
    class PartsBinPaletteWidget* m_bin;
};

class PartsBinAddRemoveArrangeCommand : public PartsBinBaseCommand
{
public:
	PartsBinAddRemoveArrangeCommand(class PartsBinPaletteWidget*, QString moduleID, QUndoCommand *parent = 0);

protected:
    QString m_moduleID;
};

class PartsBinAddRemoveCommand : public PartsBinAddRemoveArrangeCommand
{
public:
	PartsBinAddRemoveCommand(class PartsBinPaletteWidget*, QString moduleID, QString path, int index, QUndoCommand *parent = 0);

protected:
	void add();
	void remove();
    int m_index;
    QString m_path;
};

class PartsBinAddCommand : public PartsBinAddRemoveCommand
{
public:
	PartsBinAddCommand(class PartsBinPaletteWidget*, QString moduleID, QString path, int index = -1, QUndoCommand *parent = 0);
    void undo();
    void redo();
};

class PartsBinRemoveCommand : public PartsBinAddRemoveCommand
{
public:
	PartsBinRemoveCommand(class PartsBinPaletteWidget*, QString moduleID, QString path, int index = -1, QUndoCommand *parent = 0);
    void undo();
    void redo();
};

class PartsBinArrangeCommand : public PartsBinAddRemoveArrangeCommand
{
public:
	PartsBinArrangeCommand(class PartsBinPaletteWidget*, QString moduleID, int oldIndex, int newIndex, QUndoCommand *parent = 0);
    void undo();
    void redo();

protected:
	int m_oldIndex;
	int m_newIndex;
};

#endif // PARTSBINCOMMANDS_H
