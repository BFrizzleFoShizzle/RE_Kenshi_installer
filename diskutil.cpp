#include "diskutil.h"

#include <iostream>

#include "Release_Assert.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shobjidl.h>

#include <shlguid.h>

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

bool DiskUtil::CreateShortcut(std::string writePath, std::wstring cwd, std::wstring target)
{
	HRESULT hres;
	IShellLink* psl;

	// Get a pointer to the IShellLink interface. It is assumed that CoInitialize
	// has already been called.
	hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
	if (SUCCEEDED(hres))
	{
		IPersistFile* ppf;

		// Set the path to the shortcut target and add the description.
		psl->SetPath(target.c_str());
		psl->SetWorkingDirectory(cwd.c_str());

		// Query IShellLink for the IPersistFile interface, used for saving the
		// shortcut in persistent storage.
		hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);

		if (SUCCEEDED(hres))
		{
			WCHAR wsz[MAX_PATH];

			// Ensure that the string is Unicode.
			MultiByteToWideChar(CP_ACP, 0, writePath.c_str(), -1, wsz, MAX_PATH);

			// Save the link by calling IPersistFile::Save.
			hres = ppf->Save(wsz, TRUE);
			if(hres == S_OK)
				return true;
			ppf->Release();
		}
		psl->Release();
	}
	return false;
}
