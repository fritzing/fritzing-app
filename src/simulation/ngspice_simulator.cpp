/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2021-2022 Fritzing GmbH

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

#include "ngspice_simulator.h"

#include <iostream>
#include <sstream>
#include <memory>
#include <any>
#include <stdexcept>

#include <QCoreApplication>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>

#include "debugdialog.h"

// Macro for serializing variable/function name into a string.
#define STRFY(name) #name

// Macro for getting a std::function object for a ngspice library function from the map of function handlers.
#define GET_FUNC(func) std::function<decltype(func)>((decltype(func)*) m_handles[STRFY(func)])

// Macro for getting pointer to duplicated string for use in ngspice library function and automatically deleting the duplicate after the function call via unique_ptr.
#include <memory>
#include <string>
#include <cstring>

#define UNIQ(str) ([&]() {\
	auto ptr = std::make_unique<char[]>(str.size() + 1);\
	std::strcpy(ptr.get(), str.c_str());\
	return ptr.release();\
})()

NgSpiceSimulator::NgSpiceSimulator()
	: m_isInitialized(false)
	, m_isBGThreadRunning(false)
	, m_errorTitle(std::nullopt) {
}

std::shared_ptr<NgSpiceSimulator> NgSpiceSimulator::getInstance() {
	static std::shared_ptr<NgSpiceSimulator> instance;
	if (!instance) {
		instance.reset(new NgSpiceSimulator);
	}
	return instance;
}

NgSpiceSimulator::~NgSpiceSimulator() {
}

bool NgSpiceSimulator::init()
{
	if (m_isInitialized)
		return m_isInitialized;

#ifdef Q_OS_WASM
	// ngspice is linked statically into the browser build (see pri/wasm.pri)
	QString ngspiceDir("invalid");
	setErrorTitle(std::nullopt);
	m_handles[STRFY(ngSpice_Command)] = (void *) &ngSpice_Command;
	m_handles[STRFY(ngSpice_Init)] = (void *) &ngSpice_Init;
	m_handles[STRFY(ngSpice_Circ)] = (void *) &ngSpice_Circ;
	m_handles[STRFY(ngGet_Vec_Info)] = (void *) &ngGet_Vec_Info;
	m_handles[STRFY(ngSpice_SetBkpt)] = (void *) &ngSpice_SetBkpt;
	m_handles[STRFY(ngSpice_Init_Sync)] = (void *) &ngSpice_Init_Sync;
	m_handles[STRFY(ngSpice_CurPlot)] = (void *) &ngSpice_CurPlot;
	m_handles[STRFY(ngSpice_AllPlots)] = (void *) &ngSpice_AllPlots;
	m_handles[STRFY(ngSpice_AllVecs)] = (void *) &ngSpice_AllVecs;
#else
	QString ngspiceDir("invalid");
	if (!m_library.isLoaded()) {
		QStringList libPaths = QCoreApplication::libraryPaths();

#ifdef Q_OS_LINUX
		QString appDir = QCoreApplication::applicationDirPath();

		if (appDir.endsWith("/usr/bin")) {
			QString libPath = appDir.left(appDir.length() - 4) + "/lib";
			if (QDir(libPath).exists()) {
				libPaths.prepend(libPath);
			}
		}
#endif // Q_OS_LINUX


		if (DebugDialog::enabled()) {
			DebugDialog::debug("Searching for ngspice in the following directories:");
			for (const auto& path : libPaths) {
				DebugDialog::debug("  " + path);
			}
		}
#ifdef Q_OS_LINUX
		const QString libName = "libngspice.so";
#elif defined Q_OS_MACOS
		const QString libName = "libngspice.0.dylib";
#elif defined Q_OS_WIN
		const QString libName = "ngspice.dll";
#endif
		for (const auto& path : libPaths) {
			QFileInfo library(path + "/" + libName);
			if (!library.canonicalFilePath().isEmpty()) {
				m_library.setFileName(library.canonicalFilePath());
				m_library.load();
				if (m_library.isLoaded()) {
					ngspiceDir = library.absolutePath();
					break;
				}
			}
		}
		if (!m_library.isLoaded()) {
			DebugDialog::debug("Error loading ngspice shared library: " + m_library.errorString());
			throw std::runtime_error("Error loading ngspice shared library: "
									 + m_library.errorString().toStdString());
		}
	} else {
		QFileInfo loadedLibrary(m_library.fileName());
		ngspiceDir = loadedLibrary.absolutePath();
		DebugDialog::debug("ngspice already loaded: " + loadedLibrary.absoluteFilePath());
	}

	setErrorTitle(std::nullopt);

	std::vector<std::string> symbols{STRFY(ngSpice_Command), STRFY(ngSpice_Init), STRFY(ngSpice_Circ), STRFY(ngGet_Vec_Info),
									 STRFY(ngSpice_SetBkpt), STRFY(ngSpice_Init_Sync), STRFY(ngSpice_CurPlot), STRFY(ngSpice_AllPlots), STRFY(ngSpice_AllVecs)};
	for (auto & symbol: symbols) {
		m_handles[symbol] = (void *) m_library.resolve(symbol.c_str());
	}
#endif

	if (localeconv()->decimal_point[0] != '.') {
		QString errorMsg = "Error: Locale for LC_NUMERIC is not set to 'C'. Current decimal point: " +
						   QString(localeconv()->decimal_point);
		DebugDialog::debug(errorMsg);
		throw std::runtime_error(errorMsg.toStdString());
	}

	GET_FUNC(ngSpice_Init)(&SendCharFunc, &SendStatFunc, &ControlledExitFunc, &SendDataFunc, &SendInitDataFunc, &BGThreadRunningFunc, nullptr);

	m_isBGThreadRunning = true;
	m_isInitialized = true;

	QString analogCmPath = ngspiceDir + "/ngspice/analog.cm";
	if (QFileInfo::exists(analogCmPath)) {
		DebugDialog::stream() << "Loading codemodel analog.cm from: " << analogCmPath;
		command("codemodel " + analogCmPath.toStdString());
	} else {
		DebugDialog::stream() << "Warning: analog.cm not found at " << analogCmPath;
	}

	return m_isInitialized;
}

/* Callback. Set the input voltages for the external voltage sources. */
int NgSpiceSimulator::VSRCData(double* retvoltval, double acttime, char* vinstancename, int ident, void* userdata)
{
	return 0;
}

/* Callback. Set the input currents for the external current sources. */
int NgSpiceSimulator::ISRCData(double* retcurrval, double acttime, char* nodename, int ident, void* userdata)
{
	return 0;
}

/* Callback to synchronize the diiferent ngspice threads. */
int NgSpiceSimulator::SyncData(double acttime, double* deltatime, double olddeltatime,
				int redostep, int ident, int location, void* userdata)
{
	return 0;
}

void NgSpiceSimulator::resetIsBGThreadRunning() {
	m_isBGThreadRunning = true;
}

bool NgSpiceSimulator::isBGThreadRunning() {
	return m_isBGThreadRunning;
}

void NgSpiceSimulator::loadCircuit(const std::string& netList) {
	std::stringstream stream(netList);
	std::string component;
	std::vector<char *> components;
	std::vector<std::shared_ptr<char>> garbageCollector;

	while(std::getline(stream, component)) {
		auto shared = std::shared_ptr<char>(new char[component.size() + 1],
											std::default_delete<char[]>());
		std::strncpy(shared.get(), component.c_str(), component.size());
		shared.get()[component.size()] = '\0';  // Ensure null termination
		components.push_back(shared.get());
		garbageCollector.push_back(shared);
	}
	components.push_back(nullptr);
	GET_FUNC(ngSpice_Circ)(components.data());

}



void NgSpiceSimulator::command(const std::string& command) {
	// if (!m_isInitialized) {
	// 	init();
	// }
	// if (!m_isInitialized) {
	// 	return;
	// }
	m_isInitialized = !errorOccured();
	if (!m_isInitialized) {
		init();
	}

#ifdef Q_OS_WASM
	// No threads in the browser build: simulations run in the foreground and have
	// finished when the command returns
	if (command == "bg_run") {
		GET_FUNC(ngSpice_Command)(UNIQ(std::string("run")));
		m_isBGThreadRunning = false;
		return;
	}
	if (command == "bg_halt") {
		return;
	}
#endif

	GET_FUNC(ngSpice_Command)(UNIQ(command));

}

std::vector<double> NgSpiceSimulator::getVecInfo(const std::string& vecName) {
	vector_info* vecInfo = GET_FUNC(ngGet_Vec_Info)(UNIQ(vecName));

	if (!vecInfo) return std::vector<double>();

	std::vector<double> realValues;
	if (vecInfo->v_realdata) {
		for(int i=0; i<vecInfo->v_length; i++)
			realValues.push_back(vecInfo->v_realdata[i]);
		return realValues;
	}

	return std::vector<double>();
}

bool NgSpiceSimulator::setBreakPoint(const double& breakPointTime) {
	std::string previousLocale = setlocale(LC_NUMERIC, nullptr);
	setlocale(LC_NUMERIC, "C");
	std::function<bool(double)> setBkptFunc = GET_FUNC(ngSpice_SetBkpt);
	bool b = setBkptFunc(breakPointTime);
	setlocale(LC_NUMERIC, previousLocale.c_str());
	return b;
}

stdx::optional<std::string> NgSpiceSimulator::errorOccured() {
	return m_errorTitle;
}

void NgSpiceSimulator::setErrorTitle(stdx::optional<const std::reference_wrapper<std::string>> errorTitle) {
	m_errorTitle = errorTitle;
}

void NgSpiceSimulator::log(const std::string& logString, bool isStdErr) {
	if (isStdErr) {
		m_log.second += logString + "\n";
	} else {
		m_log.first += logString + "\n";
	}
}

void NgSpiceSimulator::clearLog() {
	m_log = std::make_pair<std::string, std::string>("", "");
}

std::string NgSpiceSimulator::getLog(bool isStdErr) {
	if (isStdErr) {
		return m_log.second;
	}
	return m_log.first;
}

int NgSpiceSimulator::SendCharFunc(char* output, int libId, void*) {
	std::cout << "SendCharFunc (libId:" << libId << "): " << output << std::endl;
	std::string outputStr(output);
	auto simulator = getInstance();
	if (outputStr.find("Fatal error") != std::string::npos
		|| outputStr.find("run simulation(s) aborted") != std::string::npos) {
		simulator->setErrorTitle(outputStr);
	}
	simulator->log(outputStr, outputStr.find("error") != std::string::npos);
	return 0;
}

int NgSpiceSimulator::SendStatFunc(char* simulationStatus, int libId, void*) {
	std::cout << "SendStatFunc (libId:" << libId << "): " << simulationStatus << std::endl;
	return 0;
}

int NgSpiceSimulator::ControlledExitFunc(int exitStatus, bool, bool, int libId, void*) {
	std::cout << "ControlledExitFunc exitStatus (libId:" << libId << "): " << exitStatus << std::endl;
	std::string errorTitle("Controlled Exit");
	auto simulator = getInstance();
	simulator->setErrorTitle(errorTitle);
	return 0;
}

int NgSpiceSimulator::SendDataFunc(pvecvaluesall, int numStructs, int libId, void*) {
	//std::cout << "SendDataFunc numStructs (libId:" << libId << "): " << numStructs << std::endl;
	return 0;
}

int NgSpiceSimulator::SendInitDataFunc(pvecinfoall, int libId, void*) {
	std::cout << "SendInitDataFunc (libId:" << libId << "): " << std::endl;
	return 0;
}

int NgSpiceSimulator::BGThreadRunningFunc(bool notRunning, int libId, void*) {
	std::cout << "BGThreadRunningFunc (libId:" << libId << "): " << std::endl;
	auto simulator = getInstance();
	simulator->m_isBGThreadRunning = !notRunning;
	return 0;
}

QString NgSpiceSimulator::getCurrPlot(void) {
	char * name = GET_FUNC(ngSpice_CurPlot)();
	return QString(name);
}

QList<QString> NgSpiceSimulator::getAllPlots(void) {
	char ** plots = GET_FUNC(ngSpice_AllPlots)();
	QList<QString> plotList;
	while (*plots) {
		plotList.append(QString(*plots));
		plots++;
	}
	return plotList;
}

QList<QString> NgSpiceSimulator::getAllVecs(const std::string& plotName) {
	std::string previousLocale = setlocale(LC_NUMERIC, nullptr);
	setlocale(LC_NUMERIC, "C");
	char ** vecs = GET_FUNC(ngSpice_AllVecs)(UNIQ(plotName));
	setlocale(LC_NUMERIC, previousLocale.c_str());
	QList<QString> vecList;
	while (*vecs) {
		vecList.append(QString(*vecs));
		vecs++;
	}
	return vecList;
}
