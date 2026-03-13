#ifndef DISKUTIL_H
#define DISKUTIL_H

#include <string>

namespace DiskUtil
{
    std::string GetMediaType(std::string filePath);
    bool IsOnSSD(std::string filePath);
    bool IsOnHDD(std::string filePath);
	bool CreateShortcut(std::string writePath, std::wstring cwd, std::wstring target);
}

#endif // DISKUTIL_H
