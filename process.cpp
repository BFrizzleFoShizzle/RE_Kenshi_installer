#include "process.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <RestartManager.h>

#pragma comment(lib, "Rstrtmgr.lib")

#include "bugs.h"


void ReportError(std::string step)
{
	DWORD eNum;
	TCHAR sysMsg[256];
	TCHAR* p;

	eNum = GetLastError( );
	FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				   NULL, eNum,
				   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				   sysMsg, 256, NULL );

	// Trim the end of the line and terminate it with a null
	p = sysMsg;
	while( ( *p > 31 ) || ( *p == 9 ) )
		++p;
	do { *p-- = 0; } while( ( p >= sysMsg ) &&
							( ( *p == '.' ) || ( *p < 33 ) ) );

	std::wstring werror = std::wstring(&sysMsg[0]);
	std::string error(werror.begin(), werror.end());

	Bugs::ReportBug(step, 0, "Error " + std::to_string(eNum) + " : (" + error  + ")");
	throw std::exception(("Error: " + step).c_str());
}

// taken from https://stackoverflow.com/questions/31137840/how-to-check-if-a-file-is-being-used-by-another-application
bool IsFileLocked(const std::string fileName)
{
	std::wstring wfileName(fileName.begin(), fileName.end());
	const wchar_t* PathName = wfileName.c_str();
	bool isFileLocked = false;

	DWORD dwSession = 0x0;
	wchar_t szSessionKey[CCH_RM_SESSION_KEY + 1] = { 0 };
	if ( RmStartSession( &dwSession, 0x0, szSessionKey ) == ERROR_SUCCESS )
	{
		if ( RmRegisterResources( dwSession, 1, &PathName,
								  0, NULL, 0, NULL ) == ERROR_SUCCESS )
		{
			DWORD dwReason = 0x0;
			UINT nProcInfoNeeded = 0;
			UINT nProcInfo = 0;
			if ( RmGetList( dwSession, &nProcInfoNeeded,
							&nProcInfo, NULL, &dwReason ) == ERROR_MORE_DATA )
			{
				isFileLocked = ( nProcInfoNeeded != 0 );
			}
		}
		RmEndSession( dwSession );
	}

	return isFileLocked;
}

bool IsProcessRunning(const std::wstring targetProcessName)
{
	HANDLE hProcessSnap;
	HANDLE hProcess;
	PROCESSENTRY32 pe32;
	DWORD dwPriorityClass;

	// Take a snapshot of all processes in the system.
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	// Set the size of the structure before using it.
	pe32.dwSize = sizeof(PROCESSENTRY32);

	// Retrieve information about the first process,
	// and exit if unsuccessful
	if(!Process32First(hProcessSnap, &pe32))
	{
		ReportError("IsProcessRunning_Process32First");
		CloseHandle(hProcessSnap);          // clean the snapshot object
	}

	// Now walk the snapshot of processes, and
	// display information about each process in turn
	do
	{
		std::wstring fileName = pe32.szExeFile;
		if(fileName == targetProcessName)
		{
			// found - cleanup
			CloseHandle( hProcessSnap );
			return true;
		}
	} while( Process32Next( hProcessSnap, &pe32 ) );

	// not found - cleanup
	CloseHandle(hProcessSnap);

	return false;
}
