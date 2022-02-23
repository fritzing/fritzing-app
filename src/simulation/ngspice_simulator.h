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

#ifndef NGSPICE_SIMULATOR_H
#define NGSPICE_SIMULATOR_H

#include <ngspice/sharedspice.h>

#include <map>
#include <memory>
#include <string>

#pragma once

#if __has_include(<optional>)

#include <optional>
namespace stdx {
	using std::optional;
	using std::nullopt_t;
	using std::in_place_t;
	using std::bad_optional_access;
	using std::nullopt;
	using std::in_place;
	using std::make_optional;
}

#elif __has_include(<experimental/optional>)

#include <experimental/optional>
namespace stdx {
	using std::experimental::optional;
	using std::experimental::nullopt_t;
	using std::experimental::in_place_t;
	using std::experimental::bad_optional_access;
	using std::experimental::nullopt;
	using std::experimental::in_place;
	using std::experimental::make_optional;
}

#else
	#error "an implementation of optional is required!"
#endif


#include <QLibrary>

/**
 * @brief The NgSpiceSimulator class is an interface for ngspice electronics simulation library.
 *
 * This class uses the singleton pattern.
 */
class NgSpiceSimulator {
private:
	/**
	 * @brief Private constructor. Use getInstance to get the singleton instance.
	 */
	NgSpiceSimulator();

public:
	/**
	 * @brief Return the singleton instance of NgSpiceSimulator.
	 * @return singleton instance of NgSpiceSimulator
	 */
	static std::shared_ptr<NgSpiceSimulator> getInstance();

	~NgSpiceSimulator();

	/**
	 * @brief Initialize ngspice simulator and load ngspice library.
	 */
	void init();

	/**
	 * @brief Return true if ngspice library background thread is running.
	 * @return true if ngspice library background thread is running
	 */
	bool isBGThreadRunning();

	/**
	 * @brief Load a circuit given as a netlist into the ngspice library.
	 * @param netlist that represents the circuit to be loaded into ngspice library
	 */
	void loadCircuit(const std::string& netList);
	void command(const std::string& command);
	std::vector<double> getVecInfo(const std::string& vecName);
	stdx::optional<std::string> errorOccured();
	void setErrorTitle(stdx::optional<const std::reference_wrapper<std::string>> errorTitle);
	void log(const std::string& logString, bool isStdErr);
	void clearLog();
	std::string getLog(bool isStdErr);

private:
	static int SendCharFunc(char* output, int libId, void* userData);
	static int SendStatFunc(char* simulationStatus, int libId, void* userData);
	static int ControlledExitFunc(int exitStatus, bool immediate, bool exitOnQuit, int libId, void* userData);
	static int SendDataFunc(pvecvaluesall allVecValues, int numStructs, int libId, void* userData);
	static int SendInitDataFunc(pvecinfoall allVecInitInfo, int libId, void* userData);
	static int BGThreadRunningFunc(bool notRunning, int libId, void* userData);

	std::map<std::string, void *> m_handles;
	QLibrary m_library;
	bool m_isInitialized;
	bool m_isBGThreadRunning;
	stdx::optional<std::string> m_errorTitle;
	std::pair<std::string, std::string> m_log;
};
#endif // NGSPICE_SIMULATOR_H
