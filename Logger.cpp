#include "Logger.h"


std::ofstream logFile;

void openLogFile()
{
	logFile.open("Physics_Log.txt");
	if (!logFile)
		throw "Cant open log file";
}

void logInfoIntoFile(std::string info)
{
	logFile << info << std::endl;
}