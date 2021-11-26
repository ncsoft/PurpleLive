#pragma once

//#include "graphics-hook-config.h"

#ifdef _MSC_VER
/* conversion from data/function pointer */
#pragma warning(disable : 4152)
#endif

#include "graphics-hook-info.h"
//#include <ipc-util/pipe.h>
#include <psapi.h>
#include <stdint.h>
#include "log.h"
#include "NCCaptureConfig.h"

#ifdef __cplusplus
extern "C" {
#else
#if defined(_MSC_VER) && !defined(inline)
#define inline __inline
#endif
#endif

#define NUM_BUFFERS 3

static inline const char *get_process_name(void);
static inline HMODULE get_system_module(const char *module);
static inline HMODULE load_system_library(const char *module);
extern uint64_t os_gettime_ns(void);

static inline bool capture_active(void);
static inline bool capture_ready(void);

static inline bool capture_check_target_alive(void);
static inline bool capture_should_inactive(void);
static inline bool capture_should_init(void);


extern void shmem_copy_data(size_t idx, void *volatile data);
extern bool shmem_texture_data_lock(int idx);
extern void shmem_texture_data_unlock(int idx);

extern bool hook_ddraw(void);
extern bool hook_d3d8(void);
extern bool hook_d3d9(void);
extern bool hook_dxgi(void);
extern bool hook_gl(void);

extern void unhook_d3d8(void);
extern void unhook_d3d9(void);
extern void unhook_dxgi(void);
extern void unhook_gl(void);

extern bool hook_d3d8_reset_when_jmp_patched(void);
extern bool hook_d3d9_reset_when_jmp_patched(void);
extern bool hook_dxgi_reset_when_jmp_patched(void);
extern bool hook_gl_reset_when_jmp_patched(void);

extern void d3d10_capture(void *swap, void *backbuffer, bool capture_overlay);
extern void d3d10_free(void);
extern void d3d11_capture(void *swap, void *backbuffer, bool capture_overlay);
extern void d3d11_free(void);

#if COMPILE_D3D12_HOOK
extern void d3d12_capture(void *swap, void *backbuffer, bool capture_overlay);
extern void d3d12_free(void);
#endif

extern uint8_t *get_d3d1x_vertex_shader(size_t *size);
extern uint8_t *get_d3d1x_pixel_shader(size_t *size);

extern bool rehook_gl(void);

extern bool capture_init_shtex(struct shtex_data **data, HWND window,
			       uint32_t base_cx, uint32_t base_cy, uint32_t cx,
			       uint32_t cy, uint32_t format, bool flip,
			       uintptr_t handle);
extern bool capture_init_shmem(struct shmem_data **data, HWND window,
			       uint32_t base_cx, uint32_t base_cy, uint32_t cx,
			       uint32_t cy, uint32_t pitch, uint32_t format,
			       bool flip);
extern void capture_free(void);

extern struct hook_info *global_hook_info;

struct vertex {
	struct {
		float x, y, z, w;
	} pos;
	struct {
		float u, v;
	} tex;
};

static inline bool duplicate_handle(HANDLE *dst, HANDLE src)
{
	return !!DuplicateHandle(GetCurrentProcess(), src, GetCurrentProcess(),
				 dst, 0, false, DUPLICATE_SAME_ACCESS);
}

static inline void *get_offset_addr(HMODULE module, uint32_t offset)
{
	return (void *)((uintptr_t)module + (uintptr_t)offset);
}

/* ------------------------------------------------------------------------- */

extern HANDLE pipe;
extern HANDLE signal_restart;
extern HANDLE signal_alive;
extern HANDLE signal_stop;
extern HANDLE signal_ready;
extern HANDLE signal_exit;
extern HANDLE tex_mutexes[2];
extern char system_path[MAX_PATH];
extern char process_name[MAX_PATH];
extern wchar_t keepalive_name[64];
extern HWND dummy_window;
extern volatile bool active;

static inline const char *get_process_name(void)
{
	return process_name;
}

static inline HMODULE get_system_module(const char *module)
{
	char base_path[MAX_PATH];

	strcpy(base_path, system_path);
	strcat(base_path, "\\");
	strcat(base_path, module);
	return GetModuleHandleA(base_path);
}

static inline uint32_t module_size(HMODULE module)
{
	MODULEINFO info;
	bool success = !!GetModuleInformation(GetCurrentProcess(), module,
					      &info, sizeof(info));
	return success ? info.SizeOfImage : 0;
}

static inline HMODULE load_system_library(const char *name)
{
	char base_path[MAX_PATH];
	HMODULE module;

	strcpy(base_path, system_path);
	strcat(base_path, "\\");
	strcat(base_path, name);

	module = GetModuleHandleA(base_path);
	if (module)
		return module;

	return LoadLibraryA(base_path);
}

static inline bool capture_alive(void)
{
	HANDLE handle = OpenMutexW(SYNCHRONIZE, false, keepalive_name);
	CloseHandle(handle);

	if (handle)
		return true;

	return GetLastError() != ERROR_FILE_NOT_FOUND;
}

static inline bool capture_active(void)
{
	return active;
}

static inline bool frame_ready(uint64_t interval)
{
	static uint64_t last_time = 0;
	uint64_t elapsed;
	uint64_t t;

	if (!interval) {
		return true;
	}

	t = os_gettime_ns();
	elapsed = t - last_time;

	if (elapsed < interval) {
		return false;
	}

	last_time = (elapsed > interval * 2) ? t : last_time + interval;
	return true;
}

static inline enum hook_state get_hook_state()
{
	if (global_hook_info==NULL)
		return HOOK_STATE_UNKNOWN;

	return global_hook_info->state;
}

static void set_hook_state(enum hook_state state)
{
	if (global_hook_info==NULL)	
		return;

	if(global_hook_info->state == state)
		return;

	hlog("[NCCAPTURER::hook.c!::set_hook_state] global_hook_info(0x%p) state(%d->%d)", global_hook_info, global_hook_info->state, state);	
	global_hook_info->state = state;
}

static void update_hook_ready_key(struct hook_info* pInfo)
{
	if (pInfo==NULL)
		return;

	uint32_t hook_ready_key = get_hook_ready_key(global_hook_info) + 1;

	hlog("[GBCT::HOOK::hook.c!::update_hook_ready_key] hook_ready_key(%d->%d)", pInfo->hook_ready_key, hook_ready_key);

	pInfo->hook_ready_key = hook_ready_key; 
} 

static inline bool capture_ready(void)
{
	return capture_active() &&
	       frame_ready(global_hook_info->frame_interval);
}

static inline bool capture_target_alive(void)
{
	return WaitForSingleObject(signal_alive, 0) == WAIT_OBJECT_0;
}

static inline bool capture_stopped(void)
{
	return WaitForSingleObject(signal_stop, 0) == WAIT_OBJECT_0;
}

static inline bool capture_restarted(void)
{
	return WaitForSingleObject(signal_restart, 0) == WAIT_OBJECT_0;
}

static inline bool capture_check_target_alive()
{
	if(!capture_active())
		return true;

	enum hook_state state = get_hook_state();
	bool alive = state==HOOK_STATE_ACTIVE;
	if (state==HOOK_STATE_ACTIVE)
	{
		// ACTIVE 상태 - 2초마다 alive 체크
		static uint64_t last_target_alive_check = 0;
		uint64_t cur_time = os_gettime_ns();

		if (cur_time - last_target_alive_check > 2000000000) {
			alive = capture_target_alive();
			last_target_alive_check = cur_time;
		}
	}
	else
	{
		// INACTIVE 상태 - 항시 alive 체크 (지연 없이 빠른 화면 출력을 위함)
		alive = capture_target_alive();
	}

	if (alive)
	{
		set_hook_state(HOOK_STATE_ACTIVE);
	}
	else
	{
		set_hook_state(HOOK_STATE_INACTIVE);
	}

	return alive;
}

static inline bool capture_should_inactive(void)
{
	bool need_inactive = false;

	if (capture_active())
	{
		static uint64_t last_target_alive_check = 0;
		uint64_t cur_time = os_gettime_ns();
		bool alive = true;

		if (cur_time - last_target_alive_check > 2000000000) {
			alive = capture_target_alive();
			last_target_alive_check = cur_time;
		}
		need_inactive = !alive;
	}
	
	return need_inactive;
}

extern bool init_pipe(void);

static inline bool ipc_pipe_client_valid(HANDLE pipe)
{
	return pipe != NULL && pipe != INVALID_HANDLE_VALUE;
}

static inline bool capture_should_init(void)
{
	if (!capture_active() && capture_restarted()) {
		if (capture_alive()) {
			if (!ipc_pipe_client_valid(&pipe)) {
				init_pipe();
			}
			return true;
		}
	}

	return false;
}

#ifdef __cplusplus
}
#endif
