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
	 * @param[in] netList netlist that represents the circuit to be loaded into ngspice library
	 */
	void loadCircuit(const std::string& netList);

	/**
	 * @brief Call the ngspice library ngSpice_Command function with the given command.
	 * @param[in] command command to call the ngspice library ngSpice_Command function with
	 */
	void command(const std::string& command);

	/**
	 * @brief Get vector information from the ngspice library ngGet_Vec_Info function for the given vector name.
	 * @param[in] vecName name of vector to get information for
	 * @return vector of values returned by ngspice library ngGet_Vec_Info function
	 */
	std::vector<double> getVecInfo(const std::string& vecName);

	/**
	 * @brief Return optional error title if an error occurred.
	 * @return optional error title if an error occurred
	 */
	stdx::optional<std::string> errorOccured();

	/**
	 * @brief Set the current optional error title.
	 * @param[in] errorTitle error title if an error occured, otherwise std::nullopt
	 */
	void setErrorTitle(stdx::optional<const std::reference_wrapper<std::string>> errorTitle);

	/**
	 * @brief Write stderr or stdout text to log.
	 * @param[in] logString text to log
	 * @param[in] isStdErr true if stderr, otherwise stdout
	 */
	void log(const std::string& logString, bool isStdErr);

	/**
	 * @brief Clear current stderr and stdout log.
	 */
	void clearLog();

	/**
	 * @brief Returns accumulated stderr or stdout log.
	 * @param[in] isStdErr true if stderr, otherwise stdout
	 * @return accumulated stderr or stdout log depending on isStdErr flag
	 */
	std::string getLog(bool isStdErr);

private:
	/*
	 * The following are callback functions corresponding to the typedefs in the callback section
	 * of the file sharedspice.h belonging to the ngspice library.
	 *
	 * Please refer to the parameter documentation given there.
	 */

	static int SendCharFunc(char* output, int libId, void* userData);
	static int SendStatFunc(char* simulationStatus, int libId, void* userData);
	static int ControlledExitFunc(int exitStatus, bool immediate, bool exitOnQuit, int libId, void* userData);
	static int SendDataFunc(pvecvaluesall allVecValues, int numStructs, int libId, void* userData);
	static int SendInitDataFunc(pvecinfoall allVecInitInfo, int libId, void* userData);
	static int BGThreadRunningFunc(bool notRunning, int libId, void* userData);

	/**
	 * @brief Map for handles of ngspice library functions.
	 *
	 * To get a function handle from the map use STRFY of the function name as key.
	 */
	std::map<std::string, void *> m_handles;

	/**
	 * @brief Ngspice library object.
	 */
	QLibrary m_library;

	/**
	 * @brief Flag that indicates if init() was called successfully.
	 */
	bool m_isInitialized;

	/**
	 * @brief Flag that indicates if the ngspice library background thread is running.
	 */
	bool m_isBGThreadRunning;

	/**
	 * @brief Current error title if an error occurred and otherwise std::nullopt.
	 */
	stdx::optional<std::string> m_errorTitle;

	/**
	 * @brief Pair of accumulated stdout and stderr log strings.
	 */
	std::pair<std::string, std::string> m_log;
};
#endif // NGSPICE_SIMULATOR_H
