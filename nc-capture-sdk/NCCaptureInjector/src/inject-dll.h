#pragma once
#include <windows.h>
#include <stdint.h>

enum eInjectionError {
	eIE_INJECT_FAILED,
	eIE_INVALID_PARAMS,
	eIE_OPEN_PROCESS_FAILED,
	eIE_UNLIKELY_FAIL
};

extern int inject_dll_enc(HANDLE process, const wchar_t *dll,
	const char *create_remote_thread_obf,
	uint64_t obf1,
	const char *write_process_memory_obf,
	uint64_t obf2, const char *virtual_alloc_ex_obf,
	uint64_t obf3, const char *virtual_free_ex_obf,
	uint64_t obf4, const char *load_library_w_obf,
	uint64_t obf5);

extern int inject_dll_safe_enc(DWORD thread_id, const wchar_t *dll,
	const char *set_windows_hook_ex_obf,
	uint64_t obf1);
