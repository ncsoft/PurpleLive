// NCCaptureInjector.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <windows.h>
#include "inject-dll.h"
#include "enc.h"

static void load_debug_privilege(void)
{
	const DWORD flags = TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY;
	TOKEN_PRIVILEGES tp;
	HANDLE token;
	LUID val;

	if (!OpenProcessToken(GetCurrentProcess(), flags, &token)) {
		return;
	}

	if (!!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &val)) {
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = val;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		AdjustTokenPrivileges(token, false, &tp, sizeof(tp), NULL,
			NULL);
	}

	CloseHandle(token);
}

typedef HANDLE(WINAPI* open_process_proc_t)(DWORD, BOOL, DWORD);

static inline HANDLE open_process(DWORD desired_access, bool inherit_handle, DWORD process_id)
{
	open_process_proc_t open_process_proc;
	open_process_proc = (open_process_proc_t)get_encrypted_func(GetModuleHandleW(L"KERNEL32"), "HxjcQrmkb|~", 0xc82efdf78201df87);	//OpenProcess

	return open_process_proc(desired_access, inherit_handle, process_id);
}

static inline int inject_dll(HANDLE process, const wchar_t* dll)
{
	return inject_dll_enc(process, dll, "E}mo|d[cefubWk~bgk",
		0x7c3371986918e8f6, "Rqbr`T{cnor{Bnlgwz",
		0x81bf81adc9456b35, "]`~wrl`KeghiCt",
		0xadc6a7b9acd73c9b, "Zh}{}agHzfd@{",
		0x57135138eb08ff1c, "DnafGhj}l~sX",
		0x350bfacdf81b2018);
}

static inline int inject_dll_safe(DWORD thread_id, const wchar_t* dll)
{
	return inject_dll_safe_enc(thread_id, dll, "[bs^fbkmwuKfmfOvI",	0xEAD293602FCF9778ULL);
}

static inline int inject_dll_full(DWORD process_id, const wchar_t* dll)
{
	int ret;
	HANDLE process = open_process(PROCESS_ALL_ACCESS, false, process_id);
	if (process)
	{
		ret = inject_dll(process, dll);
		CloseHandle(process);
	}
	else
	{
		ret = eIE_OPEN_PROCESS_FAILED;
	}

	return ret;
}

static int inject_helper(wchar_t* argv[], const wchar_t* dll)
{
	DWORD use_safe_inject = wcstol(argv[2], NULL, 10);
	DWORD id = wcstol(argv[3], NULL, 10);

	if (id == 0)
	{
		return eIE_INVALID_PARAMS;
	}

	return use_safe_inject ? inject_dll_safe(id, dll) : inject_dll_full(id, dll);
}

#define UNUSED_PARAMETER(x) ((void)(x))

int main(int argc, char* argv_ansi[])
{
	wchar_t dll_path[MAX_PATH];

	int ret = eIE_INVALID_PARAMS;
	SetErrorMode(SEM_FAILCRITICALERRORS);
	load_debug_privilege();

	LPWSTR pCommandLineW = GetCommandLineW();
	LPWSTR* argv = CommandLineToArgvW(pCommandLineW, &argc);
	if (argv && argc == 4)
	{
		DWORD size = GetModuleFileNameW(NULL, dll_path, MAX_PATH);
		if (size)
		{
			wchar_t* name_start = wcsrchr(dll_path, '\\');
			if (name_start)
			{
				*(++name_start) = 0;
				wcscpy(name_start, argv[1]);
				ret = inject_helper(argv, dll_path);
			}
		}
	}
	LocalFree(argv);

	UNUSED_PARAMETER(argv_ansi);
	return ret;
}
