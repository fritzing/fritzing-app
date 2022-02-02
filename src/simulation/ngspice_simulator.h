/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2021 Fritzing

********************************************************************/

#ifndef NGSPICE_SIMULATOR_H
#define NGSPICE_SIMULATOR_H

#include <ngspice/sharedspice.h>

#include <map>
#include <memory>
#include <string>

#include <QLibrary>

class NgSpiceSimulator {
private:
	NgSpiceSimulator();

public:
	static std::shared_ptr<NgSpiceSimulator> getInstance();

	~NgSpiceSimulator();

	void init();
	bool isBGThreadRunning();
	void loadCircuit(const std::string& netList);
	void command(const std::string& command);
	std::vector<double> getVecInfo(const std::string& vecName);
	std::optional<std::string> errorOccured();
	void setErrorTitle(std::optional<const std::reference_wrapper<std::string>> errorTitle);
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
	std::optional<std::string> m_errorTitle;
	std::pair<std::string, std::string> m_log;
};
#endif // NGSPICE_SIMULATOR_H
