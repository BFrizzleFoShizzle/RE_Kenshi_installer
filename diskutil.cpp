#include "diskutil.h"

#include <iostream>

#include "Release_Assert.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shobjidl.h>

#include <shlguid.h>
#include <bugs.h>

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

	hres = CoInitialize(NULL);
	if(hres == S_OK || hres == S_FALSE)
	{
		// Get a pointer to the IShellLink interface
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
				if(!SUCCEEDED(hres))
					Bugs::ReportBug("CreateShortcut_" + std::to_string(hres), 4, "Save failed");

				// cleanup
				ppf->Release();
			}
			else
			{

				Bugs::ReportBug("CreateShortcut_" + std::to_string(hres), 3, "QueryInterface failed");
			}
			psl->Release();
		}
		else
		{
			Bugs::ReportBug("CreateShortcut_" + std::to_string(hres), 2, "CoCreateInstance failed");
		}
		// CoUninitialize must be called in both S_OK and S_FALSE
		CoUninitialize();
	}
	else
	{
		Bugs::ReportBug("CreateShortcut_" + std::to_string(hres), 1, "CoInitialize failed");
	}

	return hres == S_OK;
}
