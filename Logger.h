#pragma once
#include <string>
#include <fstream>
#include <iomanip>

void logInfoIntoFile(std::string);
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#define LOG(info) (logInfoIntoFile(std::string("") + __FILENAME__ + ": " + std::to_string(__LINE__) + "\t\t| " + info))

void openLogFile();