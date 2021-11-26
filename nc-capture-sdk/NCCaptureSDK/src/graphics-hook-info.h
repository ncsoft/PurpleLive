#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "hook-helpers.h"
#include "NCCaptureConfig.h"

#define EVENT_CAPTURE_RESTART L"NCCaptureHook_Restart"
#define EVENT_CAPTURE_STOP L"NCCaptureHook_Stop"

#define EVENT_HOOK_READY L"NCCaptureHook_HookReady"
#define EVENT_HOOK_EXIT L"NCCaptureHook_Exit"

#define EVENT_HOOK_INIT L"NCCaptureHook_Initialize"
#define EVENT_HOOK_TARGETALIVE L"NCCaptureHook_TargetAlive"

#define WINDOW_HOOK_KEEPALIVE L"NCCaptureHook_KeepAlive"

#define MUTEX_TEXTURE1 L"NCCaptureHook_TextureMutex1"
#define MUTEX_TEXTURE2 L"NCCaptureHook_TextureMutex2"

#define SHMEM_HOOK_INFO L"NCCaptureHook_HookInfo"
#define SHMEM_TEXTURE L"NCCaptureHook_Texture"

#define PIPE_NAME "NCCaptureHook_Pipe"


#pragma pack(push, 8)

struct d3d8_offsets {
	uint32_t present;
};

struct d3d9_offsets {
	uint32_t present;
	uint32_t present_ex;
	uint32_t present_swap;
	uint32_t d3d9_clsoff;
	uint32_t is_d3d9ex_clsoff;
};

struct dxgi_offsets {
	uint32_t present;
	uint32_t resize;

	uint32_t present1;
};

struct ddraw_offsets {
	uint32_t surface_create;
	uint32_t surface_restore;
	uint32_t surface_release;
	uint32_t surface_unlock;
	uint32_t surface_blt;
	uint32_t surface_flip;
	uint32_t surface_set_palette;
	uint32_t palette_set_entries;
};

struct shmem_data {
	volatile int last_tex;
	uint32_t tex1_offset;
	uint32_t tex2_offset;
};

struct shtex_data {
	uint32_t tex_handle;
};

struct graphics_offsets {
	struct d3d8_offsets d3d8;
	struct d3d9_offsets d3d9;
	struct dxgi_offsets dxgi;
	struct ddraw_offsets ddraw;
};

enum capture_type {
	CAPTURE_TYPE_MEMORY,
	CAPTURE_TYPE_TEXTURE,
};

enum hook_state {
	HOOK_STATE_UNKNOWN,
	HOOK_STATE_INJECTED,
	HOOK_STATE_HOOKED,
	HOOK_STATE_ACTIVE,
	HOOK_STATE_INACTIVE
};

enum graphics_support{
	GRAPHICS_SUPPORT_DDRAW	= 0x00000001,
	GRAPHICS_SUPPORT_D3D8	= 0x00000002,
	GRAPHICS_SUPPORT_D3D9	= 0x00000004,
	GRAPHICS_SUPPORT_DXGI	= 0x00000008,
	GRAPHICS_SUPPORT_OPENGL = 0x00000010,
	GRAPHICS_SUPPORT_ALL	= 0xFFFFFFFF, 
};

struct hook_info {
	/* capture info */
	enum hook_state state;
	enum capture_type type;
	uint32_t hook_ready_key;
	enum graphics_support support;

	uint32_t window;
	uint32_t format;
	uint32_t cx;
	uint32_t cy;
	uint32_t base_cx;
	uint32_t base_cy;
	uint32_t pitch;
	uint32_t map_id;
	uint32_t map_size;
	bool flip;

	/* additional options */
	uint64_t frame_interval;
	bool use_scale;
	bool force_shmem;
	bool capture_overlay;

	/* hook addresses */
	struct graphics_offsets offsets;
};

#pragma pack(pop)

#define GC_MAPPING_FLAGS (FILE_MAP_READ | FILE_MAP_WRITE)

static inline HANDLE create_hook_info(DWORD id)
{
	wchar_t new_name[64];
	_snwprintf(new_name, 64, L"%s%lu", SHMEM_HOOK_INFO, id);

	return CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
				  sizeof(struct hook_info), new_name);
}

static uint32_t get_hook_ready_key(struct hook_info* pInfo)
{
	if (pInfo==NULL)
		return 0;

	return pInfo->hook_ready_key;
}

static bool is_hook_graphics_support_ddraw(enum graphics_support support) { return support & GRAPHICS_SUPPORT_DDRAW; }
static bool is_hook_graphics_support_d3d8(enum graphics_support support) { return support & GRAPHICS_SUPPORT_D3D8; }
static bool is_hook_graphics_support_d3d9(enum graphics_support support) { return support & GRAPHICS_SUPPORT_D3D9; }
static bool is_hook_graphics_support_dxgi(enum graphics_support support) { return support & GRAPHICS_SUPPORT_DXGI; }
static bool is_hook_graphics_support_opengl(enum graphics_support support) { return support & GRAPHICS_SUPPORT_OPENGL; }

static void set_hook_graphics_support(struct hook_info* pInfo, uint32_t support)
{
	if (pInfo==NULL)
		return;

	pInfo->support = (enum graphics_support)support; 
}