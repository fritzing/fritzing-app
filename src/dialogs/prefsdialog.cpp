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

$Revision: 6972 $:
$Author: irascibl@gmail.com $:
$Date: 2013-04-18 07:18:04 +0200 (Do, 18. Apr 2013) $

********************************************************************/

#include "prefsdialog.h"
#include "../debugdialog.h"
#include "translatorlistmodel.h"
#include "../items/itembase.h"
#include "../utils/clickablelabel.h"
#include "setcolordialog.h"
#include "../sketch/zoomablegraphicsview.h"
#include "../mainwindow/mainwindow.h"
#include "../utils/folderutils.h"

#include <QFormLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QLocale>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QSettings>
#include <QLineEdit>

#define MARGIN 5
#define FORMLABELWIDTH 195
#define SPACING 15

PrefsDialog::PrefsDialog(const QString & language, QWidget *parent)
	: QDialog(parent)
{
	this->setWindowTitle(QObject::tr("Preferences"));

	m_name = language;
	m_cleared = false;
	m_wheelMapping = (int) ZoomableGraphicsView::wheelMapping();
}

PrefsDialog::~PrefsDialog()
{
}

void PrefsDialog::initViewInfo(int index, const QString & viewName, const QString & shortName, bool curvy) 
{
	m_viewInfoThings[index].index = index;
	m_viewInfoThings[index].viewName = viewName;
	m_viewInfoThings[index].shortName = shortName;
	m_viewInfoThings[index].curvy = curvy;
}

void PrefsDialog::initLayout(QFileInfoList & languages, QList<Platform *> platforms)
{
	m_tabWidget = new QTabWidget();
	m_general = new QWidget();
	m_breadboard = new QWidget();
	m_schematic = new QWidget();
	m_pcb = new QWidget();
    m_code = new QWidget();
    m_tabWidget->setObjectName("preDia_tabs");

	m_tabWidget->addTab(m_general, tr("General"));
	m_tabWidget->addTab(m_breadboard, m_viewInfoThings[0].viewName);
	m_tabWidget->addTab(m_schematic, m_viewInfoThings[1].viewName);
	m_tabWidget->addTab(m_pcb, m_viewInfoThings[2].viewName);
    m_tabWidget->addTab(m_code, tr("Code View"));

	QVBoxLayout * vLayout = new QVBoxLayout();
	vLayout->addWidget(m_tabWidget);

    initGeneral(m_general, languages);

	initBreadboard(m_breadboard, &m_viewInfoThings[0]);
	initSchematic(m_schematic, &m_viewInfoThings[1]);
	initPCB(m_pcb, &m_viewInfoThings[2]);

    initCode(m_code, platforms);
    m_platforms = platforms;

    QDialogButtonBox * buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
	buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	vLayout->addWidget(buttonBox);

    this->setLayout(vLayout);

}

void PrefsDialog::initGeneral(QWidget * widget, QFileInfoList & languages)
{
	QVBoxLayout * vLayout = new QVBoxLayout();

	if (languages.size() > 1) {
		vLayout->addWidget(createLanguageForm(languages));
	}
	vLayout->addWidget(createColorForm());
	vLayout->addWidget(createZoomerForm());
	vLayout->addWidget(createAutosaveForm());

	vLayout->addWidget(createOtherForm());

	widget->setLayout(vLayout);
}

void PrefsDialog::initBreadboard(QWidget * widget, ViewInfoThing * viewInfoThing)
{
	QVBoxLayout * vLayout = new QVBoxLayout();

	vLayout->addWidget(createCurvyForm(viewInfoThing));
	vLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Preferred, QSizePolicy::Expanding));

	widget->setLayout(vLayout);
}

void PrefsDialog::initSchematic(QWidget * widget, ViewInfoThing * viewInfoThing)
{
	QVBoxLayout * vLayout = new QVBoxLayout();
    vLayout->addWidget(createCurvyForm(viewInfoThing));
	vLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Preferred, QSizePolicy::Expanding));

	widget->setLayout(vLayout);
}

void PrefsDialog::initPCB(QWidget * widget, ViewInfoThing * viewInfoThing)
{
	QVBoxLayout * vLayout = new QVBoxLayout();
    vLayout->addWidget(createCurvyForm(viewInfoThing));
	vLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Preferred, QSizePolicy::Expanding));

	widget->setLayout(vLayout);
}

void PrefsDialog::initCode(QWidget * widget, QList<Platform *> platforms)
{
    QVBoxLayout * vLayout = new QVBoxLayout();
    vLayout->addWidget(createProgrammerForm(platforms));
    vLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Preferred, QSizePolicy::Expanding));
    widget->setLayout(vLayout);
}

QWidget * PrefsDialog::createZoomerForm() {
	QGroupBox * zoomer = new QGroupBox(tr("Mouse Wheel Behavior"), this );

	QHBoxLayout * zhlayout = new QHBoxLayout();
	zhlayout->setSpacing(SPACING);

    QFrame * frame = new QFrame();
    frame->setFixedWidth(FORMLABELWIDTH);

    QVBoxLayout * vLayout = new QVBoxLayout();
    vLayout->setSpacing(0);
    vLayout->setMargin(0);

    for (int i = 0; i < 3; i++) {
	    m_wheelLabel[i] = new QLabel();
        vLayout->addWidget(m_wheelLabel[i]);
    }

	updateWheelText();

    frame->setLayout(vLayout);
	zhlayout->addWidget(frame);

	QPushButton * pushButton = new QPushButton(tr("Change Wheel Behavior"), this);
	connect(pushButton, SIGNAL(clicked()), this, SLOT(changeWheelBehavior()));
	zhlayout->addWidget(pushButton);

	zoomer->setLayout(zhlayout);

	return zoomer;
}

QWidget * PrefsDialog::createAutosaveForm() {
	QGroupBox * autosave = new QGroupBox(tr("Autosave"), this );

	QHBoxLayout * zhlayout = new QHBoxLayout();
	zhlayout->setSpacing(SPACING);

	QCheckBox * box = new QCheckBox(tr("Autosave every:"));
	box->setFixedWidth(FORMLABELWIDTH);
	box->setChecked(MainWindow::AutosaveEnabled);
	zhlayout->addWidget(box);

	QSpinBox * spinBox = new QSpinBox;
	spinBox->setMinimum(1);
	spinBox->setMaximum(60);
	spinBox->setValue(MainWindow::AutosaveTimeoutMinutes);
	spinBox->setMaximumWidth(80);
	zhlayout->addWidget(spinBox);

	QLabel * label = new QLabel(tr("minutes"));
	zhlayout->addWidget(label);

	zhlayout->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::Expanding));

	autosave->setLayout(zhlayout);

	connect(box, SIGNAL(clicked(bool)), this, SLOT(toggleAutosave(bool)));
	connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(changeAutosavePeriod(int)));


	return autosave;
}

QWidget * PrefsDialog::createLanguageForm(QFileInfoList & languages)
{
	QGroupBox * formGroupBox = new QGroupBox(tr("Language"));
    QVBoxLayout *layout = new QVBoxLayout();
	
	QComboBox* comboBox = new QComboBox(this);
    m_translatorListModel = new TranslatorListModel(languages, this);
	comboBox->setModel(m_translatorListModel);
	comboBox->setCurrentIndex(m_translatorListModel->findIndex(m_name));
	connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changeLanguage(int)));

	layout->addWidget(comboBox);	

	QLabel * ll = new QLabel();
	ll->setMinimumHeight(45);
	ll->setWordWrap(true);
	ll->setText(QObject::tr("Please note that a new language setting will not take effect "
		                    "until the next time you run Fritzing."));
	layout->addWidget(ll);

	formGroupBox->setLayout(layout);
	return formGroupBox;
}

QWidget* PrefsDialog::createColorForm() 
{
	QGroupBox * formGroupBox = new QGroupBox(tr("Colors"));
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setMargin(0);

    QFrame * f1 = new QFrame();
    QHBoxLayout * h1 = new QHBoxLayout();
    h1->setSpacing(SPACING);

	QLabel * c1 = new QLabel(QObject::tr("Connected highlight color"));
	c1->setWordWrap(true);
    c1->setFixedWidth(FORMLABELWIDTH);
    h1->addWidget(c1);

	QColor connectedColor = ItemBase::connectedColor();
	ClickableLabel * cl1 = new ClickableLabel(tr("%1 (click to change...)").arg(connectedColor.name()), this);
	connect(cl1, SIGNAL(clicked()), this, SLOT(setConnectedColor()));
	cl1->setPalette(QPalette(connectedColor));
	cl1->setAutoFillBackground(true);
	cl1->setMargin(MARGIN);
	h1->addWidget(cl1);

    f1->setLayout(h1);
    layout->addWidget(f1);

    QFrame * f2 = new QFrame();
    QHBoxLayout * h2 = new QHBoxLayout();
    h2->setSpacing(SPACING);

	QLabel * c2 = new QLabel(QObject::tr("Unconnected highlight color"));
	c2->setWordWrap(true);
    c2->setFixedWidth(FORMLABELWIDTH);
    h2->addWidget(c2);

	QColor unconnectedColor = ItemBase::unconnectedColor();
	ClickableLabel * cl2 = new ClickableLabel(tr("%1 (click to change...)").arg(unconnectedColor.name()), this);
	connect(cl2, SIGNAL(clicked()), this, SLOT(setUnconnectedColor()));
	cl2->setPalette(QPalette(unconnectedColor));
	cl2->setAutoFillBackground(true);
	cl2->setMargin(MARGIN);
	h2->addWidget(cl2);

    f2->setLayout(h2);
    layout->addWidget(f2);

	formGroupBox->setLayout(layout);
	return formGroupBox;
}

QWidget* PrefsDialog::createOtherForm() 
{
	QGroupBox * formGroupBox = new QGroupBox(tr("Clear Settings"));
    QHBoxLayout *layout = new QHBoxLayout();
    layout->setSpacing(SPACING);

    QVBoxLayout * vlayout = new QVBoxLayout();
    vlayout->setMargin(0);
    vlayout->setSpacing(0);

    QLabel * clearLabel = new QLabel(QObject::tr("Clear all saved settings and close this dialog immediately."));
    clearLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    clearLabel->setWordWrap(true);
    clearLabel->setFixedWidth(FORMLABELWIDTH);
    vlayout->addWidget(clearLabel);

    vlayout->addSpacing(SPACING);

    clearLabel = new QLabel(QObject::tr("This action does not delete any files; it restores settings to their default values."));
    clearLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    clearLabel->setWordWrap(true);
    clearLabel->setFixedWidth(FORMLABELWIDTH);
    vlayout->addWidget(clearLabel);

    vlayout->addSpacing(SPACING);

    clearLabel = new QLabel(QObject::tr("There is no undo for this action, and no further warning!!!!"));
    clearLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    clearLabel->setWordWrap(true);
    clearLabel->setFixedWidth(FORMLABELWIDTH);
    vlayout->addWidget(clearLabel);

    layout->addLayout(vlayout);

	QPushButton * clear = new QPushButton(QObject::tr("Clear Settings"), this);
	connect(clear, SIGNAL(clicked()), this, SLOT(clear()));

	layout->addWidget(clear);	

	formGroupBox->setLayout(layout);
	return formGroupBox;
}

QWidget* PrefsDialog::createProgrammerForm(QList<Platform *> platforms) {
    QGroupBox * formGroupBox = new QGroupBox(tr("Platform Support"));
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setSpacing(SPACING);

    foreach (Platform * platform, platforms) {
        QLabel *platformLb = new QLabel("");
        platformLb->setTextFormat(Qt::RichText);
        platformLb->setText(tr("<b>%1</b>").arg(platform->getName()));
        layout->addWidget(platformLb);

        QFrame * locationFrame = new QFrame(formGroupBox);
        QHBoxLayout * locationLayout = new QHBoxLayout();
        locationLayout->setMargin(0);
        locationLayout->setSpacing(0);
        locationFrame->setLayout(locationLayout);

        QLabel *locationLb = new QLabel(tr("Location:"));
        locationLayout->addWidget(locationLb);
        locationLayout->addSpacing(SPACING);

        QLineEdit * locationLE = new QLineEdit(locationFrame);
        locationLE->setText(platform->getCommandLocation());
        locationLayout->addWidget(locationLE);
        m_programmerLEs.insert(platform->getName(), locationLE);

        QPushButton * locateBtn = new QPushButton(tr("..."),locationFrame);
        locationLayout->addWidget(locateBtn);
        locateBtn->setProperty("platform", platform->getName());
        connect(locateBtn, SIGNAL(clicked()), this, SLOT(chooseProgrammer()));

        layout->addWidget(locationFrame);

        QLabel *hintLb = new QLabel("");
        hintLb->setTextFormat(Qt::RichText);
        hintLb->setOpenExternalLinks(true);
        hintLb->setText(tr("You need to have <a href='%1'>%2</a> (version %3 or newer) installed.")
                        .arg(platform->getDownloadUrl().toString())
                        .arg(platform->getIdeName())
                        .arg(platform->getMinVersion()));
        layout->addWidget(hintLb);

        layout->addSpacing(SPACING);
    }

    formGroupBox->setLayout(layout);
    return formGroupBox;
}

void PrefsDialog::chooseProgrammer()
{
    QObject *  button = sender();
    if (button == NULL) return;

    QString platformName = sender()->property("platform").toString();
    Platform * platform;
    foreach (Platform *p, m_platforms) {
        if (p->getName().compare(platformName) == 0) {
            platform = p;
            break;
        }
    }

    QString filename = FolderUtils::getOpenFileName(
                        this,
                        tr("Select a programmer (executable) for %1").arg(platform->getName()),
                        FolderUtils::openSaveFolder(),
                        "(Programmer *)"
                        );
    if (!filename.isEmpty()) {
        m_programmerLEs.value(platformName)->setText(filename);
        platform->setCommandLocation(filename);
    }
}

void PrefsDialog::changeLanguage(int index) 
{
	const QLocale * locale = m_translatorListModel->locale(index);
	if (locale) {
		m_name = locale->name();
		m_settings.insert("language", m_name);
	}
}

void PrefsDialog::clear() {
	m_cleared = true;
	accept();
}

bool PrefsDialog::cleared() {
	return m_cleared;
}

void PrefsDialog::setConnectedColor() {
	QColor cc = ItemBase::connectedColor();
	QColor scc = ItemBase::standardConnectedColor();

	SetColorDialog setColorDialog(tr("Connected Highlight"), cc, scc, false, this);
	int result = setColorDialog.exec();
	if (result == QDialog::Rejected) return;

	QColor c = setColorDialog.selectedColor();
	m_settings.insert("connectedColor", c.name());
	ClickableLabel * cl = qobject_cast<ClickableLabel *>(sender());
	if (cl) {
		cl->setPalette(QPalette(c));
	}
}

void PrefsDialog::setUnconnectedColor() {
	QColor cc = ItemBase::unconnectedColor();
	QColor scc = ItemBase::standardUnconnectedColor();

	SetColorDialog setColorDialog(tr("Unconnected Highlight"), cc, scc, false, this);
	int result = setColorDialog.exec();
	if (result == QDialog::Rejected) return;

	QColor c = setColorDialog.selectedColor();
	m_settings.insert("unconnectedColor", c.name());
	ClickableLabel * cl = qobject_cast<ClickableLabel *>(sender());
	if (cl) {
		cl->setPalette(QPalette(c));
	}
}

QHash<QString, QString> & PrefsDialog::settings() {
	return m_settings;
}

QHash<QString, QString> & PrefsDialog::tempSettings() {
	return m_tempSettings;
}

void PrefsDialog::changeWheelBehavior() {
	if (++m_wheelMapping >= ZoomableGraphicsView::WheelMappingCount) {
		m_wheelMapping = 0;
	}

	m_settings.insert("wheelMapping", QString("%1").arg(m_wheelMapping));
	updateWheelText();
}

void PrefsDialog::updateWheelText() {
	QString text;
#ifdef Q_OS_MAC
	QString cKey = tr("Command");
#else
	QString cKey = tr("Control");
#endif

	switch((ZoomableGraphicsView::WheelMapping) m_wheelMapping) {
		case ZoomableGraphicsView::ScrollPrimary:
		default:
			text = tr("no keys down = scroll\nshift key swaps scroll axis\nAlt or %1 key = zoom").arg(cKey);
			break;
		case ZoomableGraphicsView::ZoomPrimary:
			text = tr("no keys down = zoom\nAlt or %1 key = scroll\nshift key swaps scroll axis").arg(cKey);
			break;
	}

    QStringList strings = text.split('\n');
    for (int i = 0; i < 3; i++) {
        m_wheelLabel[i]->setText(strings.at(i));
    }
}

void PrefsDialog::toggleAutosave(bool checked) {
	m_settings.insert("autosaveEnabled", QString("%1").arg(checked));
}

void PrefsDialog::changeAutosavePeriod(int value) {
	m_settings.insert("autosavePeriod", QString("%1").arg(value));
}

QWidget* PrefsDialog::createCurvyForm(ViewInfoThing * viewInfoThing) 
{
	QGroupBox * groupBox = new QGroupBox(tr("Curvy vs. straight wires"));
    QVBoxLayout *layout = new QVBoxLayout;

    QLabel * label1 = new QLabel(tr("When you mouse-down and drag on a wire or the leg of a part (as opposed to a connector or a bendpoint) "
									"do you want to change the curvature of the wire (or leg) or drag out a new bendpoint?"));
	label1->setWordWrap(true);
	layout->addWidget(label1);

	QLabel * label2 = new QLabel(tr("This checkbox sets the default behavior. "
									"You can switch back to the non-default behavior by holding down the Control key (Mac: Command key) when you drag."));
    label2->setWordWrap(true);
	layout->addWidget(label2);

    layout->addSpacing(10);

	QCheckBox * checkbox = new QCheckBox(tr("Curvy wires and legs"));
	checkbox->setProperty("index", viewInfoThing->index);
	checkbox->setChecked(viewInfoThing->curvy);
	connect(checkbox, SIGNAL(clicked()), this, SLOT(curvyChanged()));
	layout->addWidget(checkbox);

	groupBox->setLayout(layout);
	return groupBox;
}

void PrefsDialog::curvyChanged() {
	QCheckBox * checkBox = qobject_cast<QCheckBox *>(sender());
	if (checkBox == NULL) return;

	ViewInfoThing * viewInfoThing = &m_viewInfoThings[sender()->property("index").toInt()];
	m_settings.insert(QString("%1CurvyWires").arg(viewInfoThing->shortName), checkBox->isChecked() ? "1" : "0");
}
