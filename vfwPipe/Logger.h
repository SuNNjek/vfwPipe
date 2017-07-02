//Simple Logger class for Debug purposes taken from: https://wiki.calculquebec.ca/w/C%2B%2B_:_classe_Logger/en (and slightly modified)

#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <ctime>
#include <iomanip>

#include "Helper.h"

#define LOG_DEBUG(msg) Logger::Write(Logger::DEBUG, msg, "", 0)
#define LOG_INFO(msg) Logger::Write(Logger::INFO, msg, "", 0)
#define LOG_WARN(msg) Logger::Write(Logger::WARNING, msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) Logger::Write(Logger::ERR, msg, __FILE__, __LINE__)

#define LOG_STOP Logger::Stop()

#ifdef _DEBUG
#define LOG_START Logger::Start(Logger::DEBUG, Helper::ws2s(Helper::replaceEnvVars(L"%USERPROFILE%\\vfwPipeLog.txt")))
#else
#define LOG_START Logger::Start(Logger::WARNING, Helper::ws2s(Helper::replaceEnvVars(L"%USERPROFILE%\\vfwPipeLog.txt")))
#endif


class Logger
{
public:
	// log priorities
	enum Priority
	{
		DEBUG,
		INFO,
		WARNING,
		ERR
	};

	// start/stop logging
	// - messages with priority >= minPriority will be written in log
	// - set logFile = "" to write to standard output
	static void Start(Priority minPriority, const std::string& logFile);
	static void Stop();

	// write message
	static void Write(Priority priority, const std::string& message, const std::string& file, int line);

private:
	// Logger adheres to the singleton design pattern, hence the private
	// constructor, copy constructor and assignment operator.
	Logger();
	Logger(const Logger& logger) {}
	Logger& operator = (const Logger& logger) {}

	// private instance data
	bool			active;
	std::ofstream	fileStream;
	Priority		minPriority;

	// names describing the items in enum Priority
	static const std::string PRIORITY_NAMES[];
	// the sole Logger instance (singleton)
	static Logger instance;
};