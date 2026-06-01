/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2026 Fritzing

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

********************************************************************/

#ifndef PANELIZERSTARTDIALOG_H
#define PANELIZERSTARTDIALOG_H

#include <QDialog>
#include <QString>
#include <QStringList>

class QSpinBox;
class QCheckBox;
class QRadioButton;
class QListWidget;
class QPushButton;

/**
 * @brief The small first-step panelizer dialog.
 *
 * Deliberately tiny: it asks only the two questions a user needs before
 * the real work begins — how many copies, and whether to blend in any
 * additional boards. Everything else (panel size, separation, extras,
 * arrangement, preview) lives in the single interactive window that
 * opens after this dialog is accepted. This split exists because users
 * found the previous multi-page wizard hostile; the start dialog keeps
 * the entry point trivial.
 *
 * All getters are valid only after exec() returns QDialog::Accepted.
 */
class PanelizerStartDialog : public QDialog
{
	Q_OBJECT

public:
	/**
	 * @brief Build the start dialog.
	 * @param parent           Owning widget (usually the MainWindow).
	 * @param currentSketchTitle Human-readable name of the open sketch,
	 *                          shown so the user knows what "this board"
	 *                          refers to. May be empty.
	 */
	explicit PanelizerStartDialog(QWidget * parent, const QString & currentSketchTitle);
	~PanelizerStartDialog() override = default;

	/// Number of copies of the current board to tile (1..1000).
	int copies() const;
	/// Allow the packer to rotate boards 90° to improve the fit.
	bool allowRotate() const;
	/// Absolute paths of additional .fzz boards to blend in (may be empty).
	QStringList blendPaths() const;

private slots:
	/// Enable/disable the blend list depending on the chosen mode.
	void updateBlendEnabled();
	/// Append one or more .fzz files to the blend list.
	void addBlendBoards();
	/// Remove the selected entries from the blend list.
	void removeBlendBoards();

private:
	void buildUi(const QString & currentSketchTitle);

	QSpinBox *     m_copiesSpin = nullptr;
	QCheckBox *    m_rotateCheck = nullptr;
	QRadioButton * m_modeStepRepeat = nullptr;
	QRadioButton * m_modeBlend = nullptr;
	QListWidget *  m_blendList = nullptr;
	QPushButton *  m_addButton = nullptr;
	QPushButton *  m_removeButton = nullptr;
};

#endif // PANELIZERSTARTDIALOG_H
