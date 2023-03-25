#ifndef DISKUTIL_H
#define DISKUTIL_H

#include <string>

namespace DiskUtil
{
    std::string GetMediaType(std::string filePath);
    bool IsOnSSD(std::string filePath);
    bool IsOnHDD(std::string filePath);
}

#endif // DISKUTIL_H
