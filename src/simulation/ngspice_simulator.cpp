/*******************************************************************

Part of the Fritzing project - http://fritzing.org
Copyright (c) 2021 Fritzing

********************************************************************/

#include "ngspice_simulator.h"

#include <iostream>
#include <sstream>
#include <memory>
#include <any>

#define STRFY(name) #name

#define GET_FUNC(func) std::function<decltype(func)>((decltype(func)*) m_handles[STRFY(func)])
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

	setErrorTitle(std::nullopt);

	std::vector<std::string> symbols{STRFY(ngSpice_Command), STRFY(ngSpice_Init), STRFY(ngSpice_Circ), STRFY(ngGet_Vec_Info)};
	for (auto & symbol: symbols) {
		m_handles[symbol] = (void *) m_library.resolve(symbol.c_str());
	}

	GET_FUNC(ngSpice_Init)(&SendCharFunc, &SendStatFunc, &ControlledExitFunc, &SendDataFunc, &SendInitDataFunc, &BGThreadRunningFunc, nullptr);

	m_isBGThreadRunning = true;
	m_isInitialized = true;
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
	GET_FUNC(ngSpice_Circ)(components.data());
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
	GET_FUNC(ngSpice_Command)(UNIQ(command));
}

std::vector<double> NgSpiceSimulator::getVecInfo(const std::string& vecName) {
	vector_info* vecInfo = GET_FUNC(ngGet_Vec_Info)(UNIQ(vecName));

	if (!vecInfo) return std::vector<double>();

	std::vector<double> realValues;
	if (vecInfo->v_realdata) {
		realValues.push_back(vecInfo->v_realdata[0]);
		return realValues;
	}

	return std::vector<double>();
}

std::optional<std::string> NgSpiceSimulator::errorOccured() {
	return m_errorTitle;
}

void NgSpiceSimulator::setErrorTitle(std::optional<const std::reference_wrapper<std::string>> errorTitle) {
	m_errorTitle = errorTitle;
}

int NgSpiceSimulator::SendCharFunc(char* output, int libId, void*) {
	std::cout << "SendCharFunc (libId:" << libId << "): " << output << std::endl;
	std::string outputStr(output);
	if (outputStr.find("Fatal error") != std::string::npos ||
	    outputStr.find("run simulation(s) aborted") != std::string::npos) {
		auto simulator = getInstance();
		simulator->setErrorTitle(outputStr);
	}
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
