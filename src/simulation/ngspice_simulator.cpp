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
#include "debugdialog.h"

// Macro for serializing variable/function name into a string.
#define STRFY(name) #name

// Macro for getting a std::function object for a ngspice library function from the map of function handlers.
#define GET_FUNC(func) std::function<decltype(func)>((decltype(func)*) m_handles[STRFY(func)])

// Macro for getting pointer to duplicated string for use in ngspice library function and automatically deleting the duplicate after the function call via unique_ptr.
#define UNIQ(str) std::unique_ptr<char>(strdup(str.c_str())).get()

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

void NgSpiceSimulator::init() {
	if (m_isInitialized) return;

	m_library.setFileName("ngspice");
	m_library.load();

	QStringList libPaths = QStringList({ QCoreApplication::applicationDirPath()
			})
			// TODO Not sure if we can place the library there on macOS
			+ QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation);

	if( !m_library.isLoaded() ) {         // fallback custom paths
	#ifdef Q_OS_LINUX
		const QString libName = "libngspice.so";
	#elif defined Q_OS_MACOS
		const QString libName = "libngspice.0.dylib";
	#elif defined Q_OS_WIN
		const QString libName = "ngspice.dll";
	#endif
		DebugDialog::debug("Couldn't load ngspice " + m_library.errorString());
		for( const auto& path : libPaths ) {
			QFileInfo library(QString(path + "/" + libName));
			DebugDialog::debug("Try path " + library.absoluteFilePath());
			if(!library.canonicalFilePath().isEmpty()) {
				m_library.setFileName(library.canonicalFilePath());
				m_library.load();
				if( m_library.isLoaded() ) {		
					break;
				} else {
					DebugDialog::debug("Couldn't load ngspice " + m_library.errorString());
					throw std::runtime_error( "Error loading ngspice shared library" );			
				}
			}
		}
	}

	if (!m_library.isLoaded()) {
		DebugDialog::debug("Could not find ngspice.");
		return;
	}
	DebugDialog::debug("Loaded ngspice " + m_library.fileName());


	setErrorTitle(std::nullopt);

	std::vector<std::string> symbols{STRFY(ngSpice_Command), STRFY(ngSpice_Init), STRFY(ngSpice_Circ), STRFY(ngGet_Vec_Info)};
	for (auto & symbol: symbols) {
		m_handles[symbol] = (void *) m_library.resolve(symbol.c_str());
	}
	std::string previousLocale = setlocale(LC_NUMERIC, nullptr);
	setlocale(LC_NUMERIC, "C");
	GET_FUNC(ngSpice_Init)(&SendCharFunc, &SendStatFunc, &ControlledExitFunc, nullptr, nullptr, &BGThreadRunningFunc, nullptr);
	setlocale(LC_NUMERIC, previousLocale.c_str());

	m_isBGThreadRunning = true;
	m_isInitialized = true;
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
	std::vector<std::any> garbageCollector;

	while(std::getline(stream, component)) {
		std::shared_ptr<char> shared(strdup(component.c_str()));
		components.push_back(shared.get());
		garbageCollector.push_back(shared);
	}
	components.push_back(nullptr);
	std::string previousLocale = setlocale(LC_NUMERIC, nullptr);
	setlocale(LC_NUMERIC, "C");
	GET_FUNC(ngSpice_Circ)(components.data());
	setlocale(LC_NUMERIC, previousLocale.c_str());
}

void NgSpiceSimulator::command(const std::string& command) {
	if (!m_isInitialized) {
		init();
	}
	if (!m_isInitialized) {
		return;
	}
	m_isInitialized = !errorOccured();
	if (!m_isInitialized) {
		init();
	}
	std::string previousLocale = setlocale(LC_NUMERIC, nullptr);
	setlocale(LC_NUMERIC, "C");
	GET_FUNC(ngSpice_Command)(UNIQ(command));
	setlocale(LC_NUMERIC, previousLocale.c_str());
}

std::vector<double> NgSpiceSimulator::getVecInfo(const std::string& vecName) {
	std::string previousLocale = setlocale(LC_NUMERIC, nullptr);
	setlocale(LC_NUMERIC, "C");
	vector_info* vecInfo = GET_FUNC(ngGet_Vec_Info)(UNIQ(vecName));
	setlocale(LC_NUMERIC, previousLocale.c_str());

	if (!vecInfo) return std::vector<double>();

	std::vector<double> realValues;
	if (vecInfo->v_realdata) {
		for(int i=0; i<vecInfo->v_length; i++)
			realValues.push_back(vecInfo->v_realdata[i]);
		return realValues;
	}

	return std::vector<double>();
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
	std::cout << "SendDataFunc numStructs (libId:" << libId << "): " << numStructs << std::endl;
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
