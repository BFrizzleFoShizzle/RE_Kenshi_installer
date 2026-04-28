#ifndef PROCESS_H
#define PROCESS_H
#include <string>

bool IsProcessRunning(std::wstring processName);

bool IsFileLocked(const std::wstring fileName);

#endif // PROCESS_H
