#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#else
#if defined(_MSC_VER) && !defined(inline)
#define inline __inline
#endif
#endif

#define USE_DETOUR_DUMP_LOG						1
#define DETOUR_HOOKING_MAX_RETRY_COUNT			50
#define DETOUR_JUMP_PATCHED_MAX_RETRY_COUNT		50 * 1000

struct detour_hook {
	bool init;
	bool ready;
	bool origin_jmp_patched;
	bool origin_double_patched;
	char name[32];

	void *pStatic;
	void *pOrigin;
	void *pDetour;
};

extern void detour_hook_begin();
extern void detour_hook_commit();
extern void detour_hook_attach(struct detour_hook *hook, void *pOrigin, void *pDetour, const char *name);
extern void detour_hook_detach(struct detour_hook *hook);

extern void detour_hook_stop(struct detour_hook *hook);
extern bool detour_hook_start(struct detour_hook *hook, void *pOrigin, void *pDetour, const char *name);

extern bool detour_hook_is_complete_when_jmp_patched(struct detour_hook *hook);
extern bool detour_hook_restart_when_jmp_patched(struct detour_hook *hook);

extern bool detour_hook_is_complete_when_double_patched(struct detour_hook *hook);
extern bool detour_hook_restart_when_double_patched(struct detour_hook *hook);

#ifdef __cplusplus
}
#endif
