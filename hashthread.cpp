#include "hashthread.h"

#include <QWidget>

#include <fstream>

#include "md5.h"


const std::string moddedKenshiSteamHash = "a5f78908f3f26591a6a3717bfbfafbca";
const std::string vanillaKenshiHashes[] = {
    "57679de0ae258ead45a96949974517e2", // Steam 1.0.51
    "525261fca4d339da67999c114118c6c6", // GOG 1.0.51
    "83ea507cf9667bfe8de2d8a64e9ea57a", // Steam 1.0.55
    "e737c0e734ea02fa3a714539bbb0c373",  // GOG 1.0.55
    "3327eaf22ec3c9653e22b6d7bf351736", // Steam 1.0.59
    "c74140b9ac13995500cd1413b8cc0ba2"  // GOG 1.0.59
};

bool HashThread::HashSupported(std::string hash)
{
    // I regret this
    if(hash == moddedKenshiSteamHash)
        return true;

    int numHashes = sizeof(vanillaKenshiHashes) / sizeof(vanillaKenshiHashes[0]);
    for(int i=0;i<numHashes;++i)
    {
        if(hash == vanillaKenshiHashes[i])
            return true;
    }

    return false;
}

bool HashThread::HashIsModded(std::string hash)
{
    return hash == moddedKenshiSteamHash;
}

HashThread::HashThread(std::string file, QWidget *parent)
    : QThread(parent)
{
    this->filePath = file;
}
void HashThread::run() {
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);
    if(file.is_open())
    {
        size_t size = file.tellg();
        std::vector<char> fileBytes(size);
        file.seekg(0);
        file.read(&fileBytes[0], fileBytes.size());
        md5::md5_t hasher(&fileBytes[0], fileBytes.size());
        char hashStr[33];
        hasher.get_string(hashStr);
        emit resultSuccess(hashStr);
    }
    else
    {
        emit resultError("File could not be opened");
    }
}
