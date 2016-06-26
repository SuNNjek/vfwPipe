//Simple Logger class for Debug purposes taken from: https://wiki.calculquebec.ca/w/C%2B%2B_:_classe_Logger/en (and slightly modified)

#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <ctime>
#include <iomanip>

class Logger
{
public:
	// log priorities
	enum Priority
	{
		DEBUG,
		CONFIG,
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
	static void Write(Priority priority, const std::string& message);

private:
	// Logger adheres to the singleton design pattern, hence the private
	// constructor, copy constructor and assignment operator.
	Logger();
	Logger(const Logger& logger) {}
	Logger& operator = (const Logger& logger) {}

	// private instance data
	bool        active;
	std::ofstream    fileStream;
	Priority    minPriority;

	// names describing the items in enum Priority
	static const std::string PRIORITY_NAMES[];
	// the sole Logger instance (singleton)
	static Logger instance;
};