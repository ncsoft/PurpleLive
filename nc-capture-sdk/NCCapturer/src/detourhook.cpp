#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "log.h"
#include "detourhook.h"
#include "../ext/detours/include/detours.h"

#pragma comment(lib, "ntdll.lib")

#ifdef _WIN64
	#ifdef _DEBUG
		#pragma comment(lib, "../ext/detours/lib/x64/Debug/detours.lib")
	#else
		#pragma comment(lib, "../ext/detours/lib/x64/Release/detours.lib")
	#endif
#else //_WIN64
	#ifdef _DEBUG
		#pragma comment(lib, "../ext/detours/lib/Win32/Debug/detours.lib")
	#else
		#pragma comment(lib, "../ext/detours/lib/Win32/Release/detours.lib")
	#endif
#endif //_WIN64



#define NtCurrentProcess()        ((HANDLE)(LONG_PTR)-1)
#define ZwCurrentProcess()        NtCurrentProcess()
#define NtCurrentThread()         ((HANDLE)(LONG_PTR)-2)
#define ZwCurrentThread()         NtCurrentThread()

void detour_hook_begin()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());	
}

void detour_hook_commit()
{
	DetourTransactionCommit();
}

void detour_hook_attach(struct detour_hook *hook, void *pOrigin, void *pDetour, const char *name)
{
	hook->pStatic = pOrigin;
	hook->pOrigin = pOrigin;
	hook->pDetour = pDetour;
	strcpy(hook->name, name);

	if (0==DetourAttach((PVOID*)&hook->pOrigin, hook->pDetour))
	{
		hook->ready = true;
		hook->origin_jmp_patched = 0xE9==((UINT8*)hook->pOrigin)[0];
	}
}

void detour_hook_detach(struct detour_hook *hook)
{
	DetourDetach((PVOID*)&hook->pOrigin, hook->pDetour);
}

void detour_hook_stop(struct detour_hook *hook)
{
	hlog("[NCCAPTURER::detourhook.c!::detour_hook_stop] name(%s)", hook->name);

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());	
	DetourDetach((PVOID*)&hook->pOrigin, hook->pDetour);
	DetourTransactionCommit();

	hook->ready = false;
	hook->origin_jmp_patched = false;
}

bool detour_hook_start(struct detour_hook *hook, void *pOrigin, void *pDetour, const char *name)
{
	hlog("[NCCAPTURER::detourhook.c!::detour_hook_start] name(%s) pOrigin(0x%p) pDetour(0x%p)", name, pOrigin, pDetour);

	hook->pStatic = pOrigin;
	hook->pOrigin = pOrigin;
	hook->pDetour = pDetour;
	strcpy(hook->name, name);
	hook->init = true;

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	UINT8* ptr = (UINT8*)hook->pStatic;
	UINT8 data[12];
	{
		memcpy(data, ptr, 12);
	}

	if (0==DetourAttach((PVOID*)&hook->pOrigin, hook->pDetour))
	{
		hook->ready = true;
		hook->origin_jmp_patched = 0xE9==((UINT8*)hook->pOrigin)[0];
	}

	DetourTransactionCommit();

#if USE_DETOUR_DUMP_LOG
	UINT8* p0 = data;
	UINT8* p1 = (UINT8*)hook->pStatic;
	UINT8* p2 = (UINT8*)hook->pOrigin;
	UINT8* p3 = (UINT8*)hook->pDetour;
	hlog("[NCCAPTURER::detourhook.c!::detour_hook_start] name(%s) save___(0x%p) (%02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x)", hook->name, p0, p0[0], p0[1], p0[2], p0[3], p0[4], p0[5], p0[6], p0[7], p0[8], p0[9], p0[10], p0[11] );
	hlog("[NCCAPTURER::detourhook.c!::detour_hook_start] name(%s) pStatic(0x%p) (%02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x)", hook->name, p1, p1[0], p1[1], p1[2], p1[3], p1[4], p1[5], p1[6], p1[7], p1[8], p1[9], p1[10], p1[11] );
	hlog("[NCCAPTURER::detourhook.c!::detour_hook_start] name(%s) pOrigin(0x%p) (%02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x)", hook->name, p2, p2[0], p2[1], p2[2], p2[3], p2[4], p2[5], p2[6], p2[7], p2[8], p2[9], p2[10], p2[11] );
	hlog("[NCCAPTURER::detourhook.c!::detour_hook_start] name(%s) pDetour(0x%p) (%02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x)", hook->name, p3, p3[0], p3[1], p3[2], p3[3], p3[4], p3[5], p3[6], p3[7], p3[8], p3[9], p3[10], p3[11] );
#endif

	hlog("[NCCAPTURER::detourhook.c!::detour_hook_start] name(%s) ready(%d) origin_jmp_patched(%d)", hook->name, hook->ready, hook->origin_jmp_patched);

	return hook->ready;
}

bool detour_hook_is_complete_when_jmp_patched(struct detour_hook *hook)
{
	if (!hook->init)
		return true;

	return hook->origin_jmp_patched;
}

bool detour_hook_is_complete_when_double_patched(struct detour_hook *hook)
{
	if (!hook->init)
		return true;

	return hook->origin_double_patched;
}

bool detour_hook_restart_when_jmp_patched(struct detour_hook *hook)
{
	//hlog("[NCCAPTURER::detourhook.c!::detour_hook_reset_when_jmp_patched] name(%s) pStatic(0x%p) pOrigin(0x%p) pDetour(0x%p)", hook->name, hook->pStatic, hook->pOrigin, hook->pDetour);

	if (!hook->init)
	{
		hlog("[NCCAPTURER::detourhook.c!::detour_hook_restart_when_jmp_patched] [error] init(%d) name(%s)", hook->init, hook->name);
		return false;
	}

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	if (hook->ready)
	{
		hlog("[NCCAPTURER::detourhook.c!::detour_hook_restart_when_jmp_patched] detour_hook_stop() name(%s)", hook->name);
		detour_hook_stop(hook);
	}

	UINT8* ptr = (UINT8*)hook->pStatic;
	if (0xE9!=ptr[0]) {
		//hlog("[NCCAPTURER::detourhook.c!::detour_hook_reset_when_jmp_patched] [skip] name(%s) pStatic(0x%p) (%02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x)", hook->pOrigin, name, p1[0], p1[1], p1[2], p1[3], p1[4], p1[5], p1[6], p1[7], p1[8], p1[9], p1[10], p1[11] );
		DetourTransactionCommit();
		return false;
	}
	UINT8 data[12];
	{
		memcpy(data, ptr, 12);
	}

	LONG res = DetourAttach((PVOID*)&hook->pOrigin, hook->pDetour);
	if (	res==0 &&
		data[0]==ptr[0] &&
		data[1]==ptr[1] &&
		data[2]==ptr[2] &&
		data[3]==ptr[3] &&
		data[4]==ptr[4] )
	{
		hook->ready = true;
		hook->origin_jmp_patched = true;
	}
	else
	{
		DetourDetach((PVOID*)&hook->pOrigin, hook->pDetour);
	}
	DetourTransactionCommit();

#if USE_DETOUR_DUMP_LOG
	UINT8* p0 = data;
	UINT8* p1 = (UINT8*)hook->pStatic;
	UINT8* p2 = (UINT8*)hook->pOrigin;
	UINT8* p3 = (UINT8*)hook->pDetour;
	hlog("[NCCAPTURER::detourhook.c!::detour_hook_restart_when_jmp_patched] name(%s) save___(0x%p) (%02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x)", hook->name, p0, p0[0], p0[1], p0[2], p0[3], p0[4], p0[5], p0[6], p0[7], p0[8], p0[9], p0[10], p0[11] );
	hlog("[NCCAPTURER::detourhook.c!::detour_hook_restart_when_jmp_patched] name(%s) pStatic(0x%p) (%02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x)", hook->name, p1, p1[0], p1[1], p1[2], p1[3], p1[4], p1[5], p1[6], p1[7], p1[8], p1[9], p1[10], p1[11] );
	hlog("[NCCAPTURER::detourhook.c!::detour_hook_restart_when_jmp_patched] name(%s) pOrigin(0x%p) (%02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x)", hook->name, p2, p2[0], p2[1], p2[2], p2[3], p2[4], p2[5], p2[6], p2[7], p2[8], p2[9], p2[10], p2[11] );
	hlog("[NCCAPTURER::detourhook.c!::detour_hook_restart_when_jmp_patched] name(%s) pDetour(0x%p) (%02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x)", hook->name, p3, p3[0], p3[1], p3[2], p3[3], p3[4], p3[5], p3[6], p3[7], p3[8], p3[9], p3[10], p3[11] );
#endif

	hlog("[NCCAPTURER::detourhook.c!::detour_hook_restart_when_jmp_patched] name(%s) ready(%d) origin_jmp_patched(%d)", hook->name, hook->ready, hook->origin_jmp_patched);

	return hook->ready;
}

bool detour_hook_restart_when_double_patched(struct detour_hook *hook)
{
	//hlog("[NCCAPTURER::detourhook.c!::detour_hook_restart_when_double_patched] name(%s) pStatic(0x%p) pOrigin(0x%p) pDetour(0x%p)", hook->name, hook->pStatic, hook->pOrigin, hook->pDetour);

	if (!hook->init)
	{
		hlog("[NCCAPTURER::detourhook.c!::detour_hook_restart_when_double_patched] [error] init(%d) name(%s)", hook->init, hook->name);
		return false;
	}

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	if (hook->ready)
	{
		//hlog("[NCCAPTURER::detourhook.c!::detour_hook_restart_when_double_patched] detour_hook_stop() name(%s)", hook->name);
		//detour_hook_stop(hook);
	}

	UINT8* ptr = (UINT8*)hook->pStatic;
	bool is_jmp_patched = 0xE9==ptr[0];
	if (is_jmp_patched==hook->origin_jmp_patched) {
		//hlog("[NCCAPTURER::detourhook.c!::detour_hook_restart_when_double_patched] [skip] name(%s) pStatic(0x%p) (%02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x)", hook->pOrigin, name, p1[0], p1[1], p1[2], p1[3], p1[4], p1[5], p1[6], p1[7], p1[8], p1[9], p1[10], p1[11] );
		DetourTransactionCommit();
		return false;
	}
	UINT8 data[12];
	{
		memcpy(data, ptr, 12);
	}

	LONG res = DetourAttach((PVOID*)&hook->pOrigin, hook->pDetour);
	if (res==0 &&
		data[0]==ptr[0] &&
		data[1]==ptr[1] &&
		data[2]==ptr[2] &&
		data[3]==ptr[3] &&
		data[4]==ptr[4] )
	{
		hook->ready = true;
		hook->origin_jmp_patched = true;
	}
	else
	{
		DetourDetach((PVOID*)&hook->pOrigin, hook->pDetour);
	}
	DetourTransactionCommit();

#if USE_DETOUR_DUMP_LOG
	UINT8* p0 = data;
	UINT8* p1 = (UINT8*)hook->pStatic;
	UINT8* p2 = (UINT8*)hook->pOrigin;
	UINT8* p3 = (UINT8*)hook->pDetour;
	hlog("[NCCAPTURER::detourhook.c!::detour_hook_restart_when_double_patched] name(%s) save___(0x%p) (%02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x)", hook->name, p0, p0[0], p0[1], p0[2], p0[3], p0[4], p0[5], p0[6], p0[7], p0[8], p0[9], p0[10], p0[11] );
	hlog("[NCCAPTURER::detourhook.c!::detour_hook_restart_when_double_patched] name(%s) pStatic(0x%p) (%02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x)", hook->name, p1, p1[0], p1[1], p1[2], p1[3], p1[4], p1[5], p1[6], p1[7], p1[8], p1[9], p1[10], p1[11] );
	hlog("[NCCAPTURER::detourhook.c!::detour_hook_restart_when_double_patched] name(%s) pOrigin(0x%p) (%02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x)", hook->name, p2, p2[0], p2[1], p2[2], p2[3], p2[4], p2[5], p2[6], p2[7], p2[8], p2[9], p2[10], p2[11] );
	hlog("[NCCAPTURER::detourhook.c!::detour_hook_restart_when_double_patched] name(%s) pDetour(0x%p) (%02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x)", hook->name, p3, p3[0], p3[1], p3[2], p3[3], p3[4], p3[5], p3[6], p3[7], p3[8], p3[9], p3[10], p3[11] );
#endif

	hlog("[NCCAPTURER::detourhook.c!::detour_hook_restart_when_double_patched] name(%s) ready(%d) origin_jmp_patched(%d)", hook->name, hook->ready, hook->origin_jmp_patched);

	return hook->ready;
}
