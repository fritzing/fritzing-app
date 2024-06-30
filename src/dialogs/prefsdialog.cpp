/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2007-2019 Fritzing

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

#include "prefsdialog.h"
#include "translatorlistmodel.h"
#include "../items/itembase.h"
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
#include <QString>

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

void PrefsDialog::initLayout(QFileInfoList & languages, QList<Platform *> platforms, MainWindow * mainWindow)
{
	m_mainWindow = mainWindow;
	m_projectProperties = mainWindow->getProjectProperties();
	m_tabWidget = new QTabWidget();
	m_general = new QWidget();
	m_breadboard = new QWidget();
	m_schematic = new QWidget();
	m_pcb = new QWidget();
	m_code = new QWidget();
	m_beta_features = new QWidget();
	m_tabWidget->setObjectName("preDia_tabs");

	m_tabWidget->addTab(m_general, tr("General"));
	m_tabWidget->addTab(m_breadboard, m_viewInfoThings[0].viewName);
	m_tabWidget->addTab(m_schematic, m_viewInfoThings[1].viewName);
	m_tabWidget->addTab(m_pcb, m_viewInfoThings[2].viewName);
	m_tabWidget->addTab(m_code, tr("Code View"));
	m_tabWidget->addTab(m_beta_features, tr("Beta Features"));

	auto * vLayout = new QVBoxLayout();
	vLayout->addWidget(m_tabWidget);

	initGeneral(m_general, languages);

	initBreadboard(m_breadboard, &m_viewInfoThings[0]);
	initSchematic(m_schematic, &m_viewInfoThings[1]);
	initPCB(m_pcb, &m_viewInfoThings[2]);

	initCode(m_code, platforms);
	m_platforms = platforms;

	initBetaFeatures(m_beta_features);
	auto * buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

	buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
	buttonBox->button(QDialogButtonBox::Ok)->setText(tr("OK"));

	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	vLayout->addWidget(buttonBox);

	this->setLayout(vLayout);

}

void PrefsDialog::initGeneral(QWidget * widget, QFileInfoList & languages)
{
	auto * vLayout = new QVBoxLayout();

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
	auto * vLayout = new QVBoxLayout();

	vLayout->addWidget(createCurvyForm(viewInfoThing));
	vLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Preferred, QSizePolicy::Expanding));

	widget->setLayout(vLayout);
}

void PrefsDialog::initSchematic(QWidget * widget, ViewInfoThing * viewInfoThing)
{
	auto * vLayout = new QVBoxLayout();
	vLayout->addWidget(createCurvyForm(viewInfoThing));
	vLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Preferred, QSizePolicy::Expanding));

	widget->setLayout(vLayout);
}

void PrefsDialog::initPCB(QWidget * widget, ViewInfoThing * viewInfoThing)
{
	auto * vLayout = new QVBoxLayout();
	vLayout->addWidget(createCurvyForm(viewInfoThing));
	vLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Preferred, QSizePolicy::Expanding));

	widget->setLayout(vLayout);
}

void PrefsDialog::initCode(QWidget * widget, QList<Platform *> platforms)
{
	auto * vLayout = new QVBoxLayout();
	vLayout->addWidget(createProgrammerForm(platforms));
	vLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Preferred, QSizePolicy::Expanding));
	widget->setLayout(vLayout);
}

void PrefsDialog::initBetaFeatures(QWidget * widget)
{
	QVBoxLayout * vLayout = new QVBoxLayout();
	vLayout->addWidget(createGerberBetaFeaturesForm());
	vLayout->addWidget(createProjectPropertiesForm());
	vLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Preferred, QSizePolicy::Expanding));
	widget->setLayout(vLayout);
}

QWidget * PrefsDialog::createZoomerForm() {
	auto * zoomer = new QGroupBox(tr("Mouse Wheel Behavior"), this );

	auto * zhlayout = new QHBoxLayout();
	zhlayout->setSpacing(SPACING);

	auto * frame = new QFrame();
	frame->setFixedWidth(FORMLABELWIDTH * 1.4);

	auto * vLayout = new QVBoxLayout(frame);
	vLayout->setSpacing(0);
	vLayout->setContentsMargins(0, 0, 0, 0);

	m_wheelLabel = new QLabel(frame);
	m_wheelLabel->setWordWrap(true);
	updateWheelText();

	vLayout->addWidget(m_wheelLabel);

	zhlayout->addWidget(frame);

	auto * pushButton = new QPushButton(tr("Change Wheel Behavior"), this);
	connect(pushButton, SIGNAL(clicked()), this, SLOT(changeWheelBehavior()));
	zhlayout->addWidget(pushButton);

	zoomer->setLayout(zhlayout);

	return zoomer;
}

QWidget * PrefsDialog::createAutosaveForm() {
	auto * autosave = new QGroupBox(tr("Autosave"), this );

	auto * zhlayout = new QHBoxLayout();
	zhlayout->setSpacing(SPACING);

	auto * box = new QCheckBox(tr("Autosave every:"));
	box->setFixedWidth(FORMLABELWIDTH);
	box->setChecked(MainWindow::AutosaveEnabled);
	zhlayout->addWidget(box);

	auto * spinBox = new QSpinBox;
	spinBox->setMinimum(1);
	spinBox->setMaximum(60);
	spinBox->setValue(MainWindow::AutosaveTimeoutMinutes);
	spinBox->setMaximumWidth(80);
	zhlayout->addWidget(spinBox);

	auto * label = new QLabel(tr("minutes"));
	zhlayout->addWidget(label);

	zhlayout->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::Expanding));

	autosave->setLayout(zhlayout);

	connect(box, SIGNAL(clicked(bool)), this, SLOT(toggleAutosave(bool)));
	connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(changeAutosavePeriod(int)));


	return autosave;
}

QWidget * PrefsDialog::createLanguageForm(QFileInfoList & languages)
{
	auto * formGroupBox = new QGroupBox(tr("Language"));
	auto *layout = new QVBoxLayout();

	auto* comboBox = new QComboBox(this);
	m_translatorListModel = new TranslatorListModel(languages, this);
	comboBox->setModel(m_translatorListModel);
	comboBox->setCurrentIndex(m_translatorListModel->findIndex(m_name));
	connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changeLanguage(int)));

	layout->addWidget(comboBox);

	auto * ll = new QLabel();
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
	auto * formGroupBox = new QGroupBox(tr("Colors"));
	auto *layout = new QVBoxLayout();
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);

	auto * f1 = new QFrame();
	auto * h1 = new QHBoxLayout();
	h1->setSpacing(SPACING);

	auto * c1 = new QLabel(QObject::tr("Connected highlight color"));
	c1->setWordWrap(true);
	c1->setFixedWidth(FORMLABELWIDTH);
	h1->addWidget(c1);

	QColor connectedColor = ItemBase::connectedColor();
	m_connectedColorLabel = new QLabel(QString("%1").arg(connectedColor.name()), this);
	auto * pb1 = new QPushButton(tr("%1 (click to change...)").arg(""), this);
	connect(pb1, SIGNAL(clicked()), this, SLOT(setConnectedColor()));
	m_connectedColorLabel->setPalette(QPalette(connectedColor));
	m_connectedColorLabel->setAutoFillBackground(true);
	m_connectedColorLabel->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
	h1->addWidget(m_connectedColorLabel);
	h1->addWidget(pb1);

	f1->setLayout(h1);
	layout->addWidget(f1);

	auto * f2 = new QFrame();
	auto * h2 = new QHBoxLayout();
	h2->setSpacing(SPACING);

	auto * c2 = new QLabel(QObject::tr("Unconnected highlight color"));
	c2->setWordWrap(true);
	c2->setFixedWidth(FORMLABELWIDTH);
	h2->addWidget(c2);

	QColor unconnectedColor = ItemBase::unconnectedColor();
	m_unconnectedColorLabel = new QLabel(QString("%1").arg(unconnectedColor.name()), this);
	auto * pb2 = new QPushButton(tr("%1 (click to change...)").arg(""), this);
	connect(pb2, SIGNAL(clicked()), this, SLOT(setUnconnectedColor()));
	m_unconnectedColorLabel->setPalette(QPalette(unconnectedColor));
	m_unconnectedColorLabel->setAutoFillBackground(true);
	m_unconnectedColorLabel->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
	h2->addWidget(m_unconnectedColorLabel);
	h2->addWidget(pb2);

	f2->setLayout(h2);
	layout->addWidget(f2);

	formGroupBox->setLayout(layout);
	return formGroupBox;
}

QWidget* PrefsDialog::createOtherForm()
{
	auto * formGroupBox = new QGroupBox(tr("Clear Settings"));
	auto *layout = new QVBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	auto * clearLabel = new QLabel(QObject::tr("Clear all saved settings and close this dialog immediately."));
	clearLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	clearLabel->setWordWrap(true);
	layout->addWidget(clearLabel);

	layout->addSpacing(SPACING);

	clearLabel = new QLabel(QObject::tr("This action does not delete any files; it restores settings to their default values."));
	clearLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	clearLabel->setWordWrap(true);
	layout->addWidget(clearLabel);

	layout->addSpacing(SPACING);

	clearLabel = new QLabel(QObject::tr("There is no undo for this action, and no further warning!!!!"));
	clearLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	clearLabel->setWordWrap(true);
	layout->addWidget(clearLabel);

	auto * clear = new QPushButton(QObject::tr("Clear Settings"), this);
	connect(clear, SIGNAL(clicked()), this, SLOT(clear()));

	layout->addWidget(clear);

	formGroupBox->setLayout(layout);
	return formGroupBox;
}

QWidget* PrefsDialog::createProgrammerForm(QList<Platform *> platforms) {
	auto * formGroupBox = new QGroupBox(tr("Platform Support"));
	auto *layout = new QVBoxLayout();
	layout->setSpacing(SPACING);

	Q_FOREACH (Platform * platform, platforms) {
		auto *platformLb = new QLabel("");
		platformLb->setTextFormat(Qt::RichText);
		platformLb->setText(QString("<b>%1</b>").arg(platform->getName()));
		layout->addWidget(platformLb);

		auto * locationFrame = new QFrame(formGroupBox);
		auto * locationLayout = new QHBoxLayout();
		locationLayout->setContentsMargins(0, 0, 0, 0);
		locationLayout->setSpacing(0);
		locationFrame->setLayout(locationLayout);

		auto *locationLb = new QLabel(tr("Location:"));
		locationLayout->addWidget(locationLb);
		locationLayout->addSpacing(SPACING);

		auto * locationLE = new QLineEdit(locationFrame);
		locationLE->setText(platform->getCommandLocation());
		locationLayout->addWidget(locationLE);
		m_programmerLEs.insert(platform->getName(), locationLE);

		auto * locateBtn = new QPushButton(tr("..."),locationFrame);
		locationLayout->addWidget(locateBtn);
		locateBtn->setProperty("platform", platform->getName());
		connect(locateBtn, SIGNAL(clicked()), this, SLOT(chooseProgrammer()));

		layout->addWidget(locationFrame);

		auto *hintLb = new QLabel("");
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
	if (button == nullptr) return;

	QString platformName = sender()->property("platform").toString();
	Platform * platform = nullptr;
	Q_FOREACH (Platform *p, m_platforms) {
		if (p->getName().compare(platformName) == 0) {
			platform = p;
			break;
		}
	}

	if (!platform) {
		return;
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

QWidget * PrefsDialog::createGerberBetaFeaturesForm() {
	QSettings settings;
	QGroupBox * gerberGroup = new QGroupBox(tr("Gerber"), this);

	QVBoxLayout * layout = new QVBoxLayout();

	QLabel * label2 = new QLabel(tr("The gerber file generator will use six decimals precision instead of three.\n"
									"Some deprecated gerber commands are removed or replaced.\n"
									"We recommend enabling this. Only to avoid surprises with processes that are "
									"optimized for earlier Fritzing versions, this is currently off by default."
									));
	label2->setWordWrap(true);
	layout->addWidget(label2);
	layout->addSpacing(10);

	QCheckBox * box = new QCheckBox(tr("Enable gerber export improvements"));
	box->setFixedWidth(FORMLABELWIDTH * 2);
	box->setChecked(settings.value("gerberExportImprovementsEnabled", false).toBool()); // Initialize the value of box2 using m_settings
	layout->addWidget(box);


	gerberGroup->setLayout(layout);

	connect(box, &QCheckBox::clicked, this, [this](bool checked) {
		m_settings.insert("gerberExportImprovementsEnabled", QString::number(checked));
	});

	return gerberGroup;
}

QWidget *PrefsDialog::createProjectPropertiesForm() {
	QGroupBox * projectPropertiesBox = new QGroupBox(tr("Project properties"), this );

	QVBoxLayout * layout = new QVBoxLayout();
	layout->setSpacing(SPACING);

	QLabel * label = new QLabel(tr("Here you can set some settings that will be saved with the project"));
	label->setWordWrap(true);
	layout->addWidget(label);
	layout->addSpacing(10);

    bool timeStepMode = m_projectProperties->getProjectProperty(ProjectPropertyKeySimulatorTimeStepMode).toLower().contains("true");
    QLabel *simTimeStepModeLabel = new QLabel(tr("Select the way to define the time step: (1) "
                                                 "Number of points (max simulation time divided by the number of points) "
                                                 "or (2) fixed time step."));
    simTimeStepModeLabel->setWordWrap(true);
    layout->addWidget(simTimeStepModeLabel);
    QHBoxLayout * simStepslayout = new QHBoxLayout();
    QRadioButton *simPointsRB = new QRadioButton("Number of Points (recommended)");
    simPointsRB->setChecked(!timeStepMode);
    simStepslayout->addWidget(simPointsRB);
    simStepslayout->addSpacing(1);
    QLabel * numPointslabel = new QLabel(tr("Number of points: "));
    simStepslayout->addWidget(numPointslabel);
    QLineEdit *simNumStepsEdit = new QLineEdit();
    simNumStepsEdit->setToolTip("The time step is calculated as the simulation time divided by the numper of steps\n"
                                 "Low number of steps could cause inestabilities in the simulation and render artifacts in the oscilloscope.");
    simStepslayout->addWidget(simNumStepsEdit);
    simNumStepsEdit->setText(m_projectProperties->getProjectProperty(ProjectPropertyKeySimulatorNumberOfSteps));

    layout->addLayout(simStepslayout);

    QHBoxLayout * simTimeSteplayout = new QHBoxLayout();
    QRadioButton *simTimeStepRB = new QRadioButton("Fixed Time Step");
    simTimeStepRB->setChecked(timeStepMode);
    simTimeSteplayout->addWidget(simTimeStepRB);
    simTimeSteplayout->addSpacing(10);
    QLabel *simTimeStepLabel = new QLabel(tr("Time Step (s):"));
    simTimeSteplayout->addWidget(simTimeStepLabel);
    QLineEdit *simTimeStepEdit = new QLineEdit();

    simTimeStepEdit->setText(m_projectProperties->getProjectProperty(ProjectPropertyKeySimulatorTimeStepS));
    simTimeStepEdit->setToolTip("The time step to be used in transitory simulations.\n"
                                "Small time steps and long simulations could cause prformance issues.");
    simTimeSteplayout->addWidget(simTimeStepEdit);

    layout->addLayout(simTimeSteplayout);

    QLabel * simAnimationTimelabel = new QLabel(tr("Animation time for the transitory simulation (s): "));
    layout->addWidget(simAnimationTimelabel);
    QLineEdit *simAnimationTimeEdit = new QLineEdit();
    simAnimationTimeEdit->setText(m_projectProperties->getProjectProperty(ProjectPropertyKeySimulatorAnimationTimeS));
    simAnimationTimeEdit->setFixedWidth(FORMLABELWIDTH * 2);
    simAnimationTimeEdit->setToolTip("This is the time used to animate the effects of a transitory simulation.\n"
                                     "Set it to 0 if you do not want an animation.");
    layout->addWidget(simAnimationTimeEdit);

    projectPropertiesBox->setLayout(layout);

    connect(simTimeStepRB, SIGNAL(toggled(bool)), this, SLOT(setSimulationTimeStepMode(bool)));
    connect(simNumStepsEdit, SIGNAL(textChanged(QString)), this, SLOT(setSimulationNumberOfSteps(QString)));
    connect(simTimeStepEdit, SIGNAL(textChanged(QString)), this, SLOT(setSimulationTimeStep(QString)));
    connect(simAnimationTimeEdit, SIGNAL(textChanged(QString)), this, SLOT(setSimulationAnimationTime(QString)));

	return projectPropertiesBox;

}

void PrefsDialog::setSimulationTimeStepMode(const bool &timeStepMode) {
    QString mode = timeStepMode ? "true" : "false";
    m_projectProperties->setProjectProperty(ProjectPropertyKeySimulatorTimeStepMode, mode);
}

void PrefsDialog::setSimulationNumberOfSteps(const QString &numberOfSteps) {
    m_projectProperties->setProjectProperty(ProjectPropertyKeySimulatorNumberOfSteps, numberOfSteps);
}

void PrefsDialog::setSimulationTimeStep(const QString &timeStep) {
    m_projectProperties->setProjectProperty(ProjectPropertyKeySimulatorTimeStepS, timeStep);
}

void PrefsDialog::setSimulationAnimationTime(const QString &animationTime) {
    m_projectProperties->setProjectProperty(ProjectPropertyKeySimulatorAnimationTimeS, animationTime);
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
	if (m_connectedColorLabel) {
		m_connectedColorLabel->setPalette(QPalette(c));
		m_connectedColorLabel->setText(QString("%1").arg(c.name()));
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
	if (m_unconnectedColorLabel) {
		m_unconnectedColorLabel->setPalette(QPalette(c));
		m_unconnectedColorLabel->setText(QString("%1").arg(c.name()));
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
#ifdef Q_OS_MACOS
	QString cKey = tr("Command");
#else
	QString cKey = tr("Control");
#endif

	switch((ZoomableGraphicsView::WheelMapping) m_wheelMapping) {
	case ZoomableGraphicsView::ScrollPrimary:
	default:
		text = tr("<b>Scroll priority</b><br/>") + tr("no keys down = scroll<br/><kbd>Shift</kbd> key swaps scroll axis<br/><kbd>Alt</kbd> or <kbd>%1</kbd> = zoom").arg(cKey);
		break;
	case ZoomableGraphicsView::ZoomPrimary:
		text = tr("<b>Zoom priority</b><br/>") + tr("no keys down = zoom<br/><kbd>Alt</kbd> or <kbd>%1</kbd> = scroll<br/><kbd>Shift</kbd> key swaps scroll axis").arg(cKey);
		break;
	case ZoomableGraphicsView::Guess:
		text = tr("<b>Guess</b><br/>") +
				tr("Let Fritzing guess if the input is from a wheel or a touchpad. <kbd>Alt</kbd> or <kbd>%1</kbd> modify scrolling. <kbd>Shift</kbd> can modify the axis or the speed.").arg(cKey);
		break;
	case ZoomableGraphicsView::Pure:
		text = tr("<b>Pure</b><br/>") + tr("Use system defaults to interpret the wheel input. Don't try anything fancy. Recommended when using a touchpad with pinch gestures.");
	}

	// border, border-radius and padding are not supported
	QString css = "<style>"
				  "kbd { "
				  "    font-size:14; "
				  "    color: black; "
				  "    background-color: grey; "
				  "    border: 1px solid black; "
				  "    border-radius: 5px; "
				  "    padding: 2px; "
				  "}"
				  "</style>";

	m_wheelLabel->setText(css + text);
}

void PrefsDialog::toggleAutosave(bool checked) {
	m_settings.insert("autosaveEnabled", QString("%1").arg(checked));
}

void PrefsDialog::changeAutosavePeriod(int value) {
	m_settings.insert("autosavePeriod", QString("%1").arg(value));
}

QWidget* PrefsDialog::createCurvyForm(ViewInfoThing * viewInfoThing)
{
	auto * groupBox = new QGroupBox(tr("Curvy vs. straight wires"));
	auto *layout = new QVBoxLayout;

	auto * label1 = new QLabel(tr("When you mouse-down and drag on a wire or the leg of a part (as opposed to a connector or a bendpoint) "
	                                "do you want to change the curvature of the wire (or leg) or drag out a new bendpoint?"));
	label1->setWordWrap(true);
	layout->addWidget(label1);

	auto * label2 = new QLabel(tr("This checkbox sets the default behavior. "
	                                "You can switch back to the non-default behavior by holding down the Control key (Mac: Command key) when you drag."));
	label2->setWordWrap(true);
	layout->addWidget(label2);

	layout->addSpacing(10);

	auto * checkbox = new QCheckBox(tr("Curvy wires and legs"));
	checkbox->setProperty("index", viewInfoThing->index);
	checkbox->setChecked(viewInfoThing->curvy);
	connect(checkbox, SIGNAL(clicked()), this, SLOT(curvyChanged()));
	layout->addWidget(checkbox);

	groupBox->setLayout(layout);
	return groupBox;
}

void PrefsDialog::curvyChanged() {
	auto * checkBox = qobject_cast<QCheckBox *>(sender());
	if (checkBox == nullptr) return;

	ViewInfoThing * viewInfoThing = &m_viewInfoThings[sender()->property("index").toInt()];
	m_settings.insert(QString("%1CurvyWires").arg(viewInfoThing->shortName), checkBox->isChecked() ? "1" : "0");
}
