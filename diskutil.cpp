#include "diskutil.h"

#include <iostream>

#include "Release_Assert.h"

// Adapted from https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
std::string exec(std::string cmd) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe)
        return "POPEN_ERROR";
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        _pclose(pipe);
        throw;
    }
    _pclose(pipe);
    return result;
}

std::string DiskUtil::GetMediaType(std::string filePath)
{
    // Extract drive letter
    assert(isalpha(filePath[0]));
    assert(filePath[1] == ':');
    std::string driveLetter = filePath.substr(0,1);


    // Powershell blob for checking the type of a drive
    std::string powershellCommand = "(Get-PhysicalDisk -UniqueId ((Get-Partition -DriveLetter " + driveLetter + " | Select UniqueId).UniqueId.split('}')[1]) | Select MediaType).MediaType";

    std::string output = exec("powershell.exe -windowstyle hidden \"" + powershellCommand + "\"");

    return output.substr(0, output.find("\n"));
}

bool DiskUtil::IsOnSSD(std::string filePath)
{
    std::string mediaType = GetMediaType(filePath);
    if(mediaType == "SSD")
        return true;

    return false;
}


bool DiskUtil::IsOnHDD(std::string filePath)
{
    std::string mediaType = GetMediaType(filePath);

    std::cout << mediaType;

    if(mediaType == "HDD")
        return true;

    return false;
}
