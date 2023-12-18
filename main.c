#include <Windows.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

#include "CommandLine.h"
#include "ToInteger.h"

#ifndef _T
#	ifdef _UNICODE
#		define _T(s) L ## s
#	else
#		define _T(s) s
#	endif
#endif

#ifdef _DEBUG
#	include <stdio.h>
#	ifdef _UNICODE
#		define _dbglog(format, ...) wprintf(_T("debug: ") format, __VA_ARGS__)
#	else
#		define _dbglog(format, ...) printf(_T("debug: ") format, __VA_ARGS__)
#	endif
#else
#	define _dbglog(s, ...)
#endif

#define _errwrite(s) WriteConsole(_std_handle.error, s, lstrlen(s), NULL, NULL);
#define _write(s) WriteConsole(_std_handle.output, s, lstrlen(s), NULL, NULL)
#define _cwrite(s) WriteConsole(_std_handle.output, s, sizeof(s) / sizeof(s[0]), NULL, NULL)

struct {
	HANDLE output;
	HANDLE input;
	HANDLE error;
} _std_handle;

struct
{
	unsigned int dlls;
	BOOL cmd;
	BOOL exe;
	BOOL dbgsus;
	BOOL sdir;
} _args;

enum
{
	E_NEATW = INT_MIN,
	E_HELP,
	E_UTCP,
	E_RTF,
	E_CCRT,
	E_CWDM,
	E_UAVMP
};

#ifndef _INC_MEMORY
void* memset(void* dest, int val, unsigned int len);
#endif

void exception(LPCTSTR s, int code);

void inject(LPTSTR dll, PROCESS_INFORMATION pi);

void main()
{
	_std_handle.error = GetStdHandle(STD_ERROR_HANDLE);
	_std_handle.input = GetStdHandle(STD_INPUT_HANDLE);
	_std_handle.output = GetStdHandle(STD_OUTPUT_HANDLE);

	unsigned int argc;

	LPTSTR cmdline = GetCommandLine();

	_dbglog(_T("CmdLine: %s\n"), cmdline);

	LPTSTR* argv = CommandLineToArgv(cmdline, &argc);

#ifdef _DEBUG
	_dbglog(_T("Count of args: %d\n"), argc);
	_dbglog(_T("List of args:\n"));
	for (unsigned int i = 0; i < argc; i++)
		_dbglog(_T("%d: %s\n"), i, argv[i]);
#endif

	if (argc == 2 && !lstrcmp(argv[1], _T("-help")))
		exception(
			_T("Arguments:\n")
			_T("-dbgsus -> Pause program before injecting dll to attach debugger\n")
			_T("-cmd <cmd> -> You can run the program with parameters\n")
			_T("-sdir <path-to-dir|auto> -> The process will be created in the needed directory\n")
			_T("-exe <path-to-exe> -> Process for injection\n") 
			_T("-dlls <count> <dll> <dll>... -> DLLs for inject into the process\n\n")
			_T("Examples:\n")
			_T("injector.exe -dbgsus -cmd \"C:\\hl\\hl.exe -game cstrike\" -dlls 1 hack.dll -sdir auto\n")
			_T("injector.exe -exe C:\\game\\game.exe -dlls 3 plug1.dll plug2.dll C:\\game\\cheat\\plug3.dll\n"),
			E_HELP
		);

	if (argc <= 3)
		exception(
			_T("Not enough arguments to work.\n")
			_T("Use: Injector.exe -help\n"), 
			E_NEATW
		);

	_args.sdir = FALSE;
	_args.dbgsus = FALSE;
	_args.cmd = FALSE;
	_args.exe = FALSE;
	_args.dlls = 0;

	TCHAR executable[MAX_PATH];
	TCHAR startdir[MAX_PATH];

	ZeroMemory(&startdir, MAX_PATH);

	LPTSTR* dlls = NULL;
	unsigned int dlls_counter = 0;

	for (unsigned int i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
			goto command;

		if (_args.dlls != 0 && dlls_counter < _args.dlls)
		{
			TCHAR buffer[MAX_PATH] = { 0 };
			GetFullPathName(argv[i], MAX_PATH, buffer, NULL);
			_dbglog(_T("Full path of finded dll(%d): %s\n"), dlls_counter, buffer);
			dlls[dlls_counter++] = buffer;
		}

		continue;
	command:

		if (!lstrcmp(argv[i], _T("-dbgsus")) && !_args.dbgsus)
		{
			_args.dbgsus = TRUE;
			_dbglog(_T("Debug mode enabled.\n"));
		}
		else if (!lstrcmp(argv[i], _T("-exe")) && !_args.exe)
		{
			_args.exe = TRUE;
			GetFullPathName(argv[++i], MAX_PATH, executable, NULL);
			_dbglog(_T("Full path executable: %s\n"), executable);
		}
		else if (!lstrcmp(argv[i], _T("-cmd")) && !_args.cmd)
		{
			_args.cmd = TRUE;
			_dbglog(_T("CMD mode enabled.\n"));
			lstrcpy(executable, argv[++i]);
			_dbglog(_T("Cmd entry: %s\n"), executable);
		}
		else if (!lstrcmp(argv[i], _T("-sdir")) && !_args.sdir)
		{
			_args.sdir = TRUE;
			_dbglog(_T("Start dir mode enabled.\n"));
			lstrcpy(startdir, argv[++i]);
			_dbglog(_T("Start directory: %s\n"), startdir);
		}
		else if (!lstrcmp(argv[i], _T("-dlls")) && _args.dlls == 0)
		{
			_dbglog(_T("Dlls inject mode enabled.\n"));
			_args.dlls = to_integer((char*)argv[++i]);
			dlls = (LPTSTR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, _args.dlls * sizeof(LPTSTR));
			for (unsigned int j = 0; j < _args.dlls; j++)
				dlls[j] = (LPTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH * sizeof(TCHAR));
			_dbglog(_T("Count of dlls: %d\n"), _args.dlls);
		}
	}

	LocalFree(argv);

	_dbglog(_T("Start.\n"));

	LPTSTR directory = NULL;

	if (_args.sdir)
	{
		if (!lstrcmp(startdir, _T("auto")))
		{
			ZeroMemory(&startdir, MAX_PATH);
			for (int i = 0; i < StrRChr(executable, NULL, '\\') - executable + 1; i++)
				startdir[i] = executable[i];
		}
		directory = startdir;
	}

	STARTUPINFO si = { 0 };
	ZeroMemory(&si, sizeof(si));

	si.cb = sizeof(STARTUPINFO);

	PROCESS_INFORMATION pi = { 0 };
	ZeroMemory(&pi, sizeof(pi));

	BOOL result;
	if (_args.cmd == TRUE)
		result = CreateProcess(NULL, executable, NULL, NULL, FALSE, CREATE_SUSPENDED | CREATE_NEW_CONSOLE, NULL, directory, &si, &pi);
	else
		result = CreateProcess(executable, NULL, NULL, NULL, FALSE, CREATE_SUSPENDED | CREATE_NEW_CONSOLE, NULL, directory, &si, &pi);

	if (!result)
		exception(_T("Unable to create process"), E_UTCP);

	if (_args.dbgsus)
	{
		_cwrite(_T("Paused. You can attach debugger.\nPress ENTER for resume.\n"));
		WaitForSingleObject(_std_handle.input, INFINITE);
	}

	for (unsigned int i = 0; i < dlls_counter; i++)
		inject(dlls[i], pi);

	if (ResumeThread(pi.hThread) == -1)
		exception(_T("Failed to resume the thread"), E_RTF);

	_dbglog(_T("End.\n"));
	
	ExitProcess(0);
}

void exception(LPCTSTR s, int code)
{
	_errwrite(s);
	ExitProcess(code);
}

void inject(LPTSTR dll, PROCESS_INFORMATION pi)
{
	void* loc = VirtualAllocEx(pi.hProcess, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	if (loc)
	{
		SIZE_T bytes = 0;
		unsigned int len = lstrlen(dll);

		WriteProcessMemory(pi.hProcess, loc, dll, len + 1, &bytes);

		if (bytes)
		{
			HANDLE hThread = CreateRemoteThread(pi.hProcess, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibrary, loc, 0, 0);

			if (hThread != INVALID_HANDLE_VALUE && hThread != 0) {

				CloseHandle(hThread);

				_write(StrRChr(dll, NULL, '\\') + 1);
				_cwrite(_T(" injected!\n"));
			}
			else exception(_T("Cannot create remote thread\n"), E_CCRT);
		}
		else exception(_T("Cannot write data to memory\n"), E_CWDM);
	}
	else exception(_T("Unable to allocate virtual memory in process\n"), E_UAVMP);
}

#pragma function(memset)
void* memset(void* dest, int val, unsigned int len)
{
	unsigned char* ptr = dest;
	while (len-- > 0)
		*ptr++ = val;
	return dest;
}