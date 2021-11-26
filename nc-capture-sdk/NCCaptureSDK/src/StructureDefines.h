#pragma once

#include <Windows.h>
#include <stdint.h>
#include <string>

struct gs_texture;
enum eColorFormat 
{
	eCF_UNKNOWN,
	eCF_A8,
	eCF_R8,
	eCF_RGBA,
	eCF_BGRX,
	eCF_BGRA,
	eCF_R10G10B10A2,
	eCF_RGBA16,
	eCF_R16,
	eCF_RGBA16F,
	eCF_RGBA32F,
	eCF_RG16F,
	eCF_RG32F,
	eCF_R16F,
	eCF_R32F,
	eCF_DXT1,
	eCF_DXT3,
	eCF_DXT5,
	eCF_R8G8,
};

enum hook_rate {
	HOOK_RATE_SLOW,
	HOOK_RATE_NORMAL,
	HOOK_RATE_FAST,
	HOOK_RATE_FASTEST
};

struct game_capture_config {
	char *szTitle;
	char *szClass;
	char *szExe;
	uint32_t scale_cx;
	uint32_t scale_cy;
	bool cursor;
	bool force_shmem;
	bool force_scaling;
	bool allow_transparency;
	bool limit_framerate;
	bool capture_overlays;
	bool anticheat_hook;
	enum hook_rate hook_rate;
};

class Texture2D;
struct GameCapture
{
	HANDLE injector_process;
	uint32_t cx;
	uint32_t cy;
	uint32_t pitch;
	DWORD		process_id;
	DWORD		thread_id;
	HWND		window;
	float		retry_time;
	float		fps_reset_time;
	float		retry_interval;
	std::string	strTitle;
	std::string	strClass;
	std::string strExe;

	volatile bool deactivate_hook;
	volatile bool activate_hook_now;
	bool wait_for_target_startup;
	bool showing;
	bool active;
	bool capturing;
	bool activate_hook;
	bool process_is_64bit;
	bool error_acquiring;
	bool dwm_capture;
	bool initial_config;
	bool convert_16bit;
	bool is_app;
	bool cursor_hidden;

	struct game_capture_config config;

//	ipc_pipe_server_t pipe;
//	gs_texture *texture;
	Texture2D*	texture;
	struct hook_info *global_hook_info;
	uint32_t hook_ready_key;
	HANDLE keepalive_mutex;

	HANDLE hook_init;
	HANDLE hook_restart;
	HANDLE hook_alive;
	HANDLE hook_stop;
	HANDLE hook_ready;
	HANDLE hook_exit;

	HANDLE hook_data_map;
	HANDLE global_hook_info_map;
	HANDLE target_process;
	HANDLE texture_mutexes[2];
	wchar_t *app_sid;
	int retrying;
	float cursor_check_time;

	union {
		struct {
			struct shmem_data *shmem_data;
			uint8_t *texture_buffers[2];
		};

		struct shtex_data *shtex_data;
		void *data;
	};

	bool is_hooked()
	{
		if (global_hook_info==NULL)
			return false;
		
		enum hook_state state = global_hook_info->state;
		if (state==HOOK_STATE_HOOKED || state==HOOK_STATE_ACTIVE || state==HOOK_STATE_INACTIVE)
			return true;

		return false;
	}

	void(*copy_texture)(struct GameCapture*);
};

