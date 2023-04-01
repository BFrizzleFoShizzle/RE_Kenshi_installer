#ifndef PROCESS_H
#define PROCESS_H
#include <string>

bool IsProcessRunning(const std::wstring processName);

bool IsFileLocked(const std::string fileName);

#endif // PROCESS_H
