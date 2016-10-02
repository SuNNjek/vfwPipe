#include "Logger.h"

// --------------------------------------
// static members initialization
// --------------------------------------

const std::string Logger::PRIORITY_NAMES[] =
{
	"DEBUG",
	"INFO",
	"WARNING",
	"ERROR"
};

Logger Logger::instance;


// --------------------------------------
// function implementations
// --------------------------------------

Logger::Logger() : active(false) {}

void Logger::Start(Priority minPriority, const std::string& logFile)
{
	instance.active = true;
	instance.minPriority = minPriority;
	if (logFile != "")
	{
		instance.fileStream.open(logFile.c_str(), std::ios::out | std::ios::app);
	}
}

void Logger::Stop()
{
	instance.active = false;
	if (instance.fileStream.is_open())
	{
		instance.fileStream.close();
	}
}

void Logger::Write(Priority priority, const std::string& message, const std::string& file, int line)
{
	if (instance.active && priority >= instance.minPriority)
	{
		// identify current output stream
		std::ostream& stream
			= instance.fileStream.is_open() ? instance.fileStream : std::cout;

		time_t t;
		time(&t);
		tm time;
		localtime_s(&time, &t);

		stream << "["
			<< std::put_time(&time, "%d.%m.%Y %T")
			<< "]["
			<< PRIORITY_NAMES[priority]
			<< "]"
			<< ": "
			<< message;

		if (file != "")
			stream << " (at " << file << ":" << line << ")";

		stream << std::endl;
	}
}
