#include <windows.h>
#include <psapi.h>
#include "graphics-hook.h"
#include "enc.h"
#include "detourhook.h"
#include "pch.h"
#include "offsets/get-graphics-offsets.h"
#include "NCCaptureConfig.h"

struct thread_data {
	CRITICAL_SECTION mutexes[NUM_BUFFERS];
	CRITICAL_SECTION data_mutex;
	void *volatile cur_data;
	uint8_t *shmem_textures[2];
	HANDLE copy_thread;
	HANDLE copy_event;
	HANDLE stop_event;
	volatile int cur_tex;
	unsigned int pitch;
	unsigned int cy;
	volatile bool locked_textures[NUM_BUFFERS];
};

HANDLE pipe = {0};
HANDLE signal_restart = NULL;
HANDLE signal_alive = NULL;
HANDLE signal_stop = NULL;
HANDLE signal_ready = NULL;
HANDLE signal_exit = NULL;
static HANDLE signal_init = NULL;
HANDLE tex_mutexes[2] = {NULL, NULL};
static HANDLE filemap_hook_info = NULL;

static HINSTANCE dll_inst = NULL;
static volatile bool stop_loop = false;
static HANDLE capture_thread = NULL;
char system_path[MAX_PATH] = {0};
char process_name[MAX_PATH] = {0};
wchar_t keepalive_name[64] = {0};
HWND dummy_window = NULL;

static unsigned int shmem_id = 0;
static void *shmem_info = NULL;
static HANDLE shmem_file_handle = 0;

static struct thread_data thread_data = {0};

volatile bool active = false;
struct hook_info *global_hook_info = NULL;

static inline void wait_for_dll_main_finish(HANDLE thread_handle)
{
	if (thread_handle) {
		WaitForSingleObject(thread_handle, 100);
		CloseHandle(thread_handle);
	}
}

bool init_pipe(void)
{
	char new_name[64];
	sprintf(new_name, "%s%lu", PIPE_NAME, GetCurrentProcessId());

	// TODO
	/*
	if (!ipc_pipe_client_open(&pipe, new_name)) {
		DbgOut("Failed to open pipe");
		return false;
	}
	*/
	return true;
}

static HANDLE init_event(const wchar_t *name, DWORD pid)
{
	HANDLE handle = create_event_plus_id(name, pid);
	if (!handle)
		hlog("Failed to get event '%s': %lu", name, GetLastError());
	return handle;
}

static HANDLE init_mutex(const wchar_t *name, DWORD pid)
{
	HANDLE handle = create_mutex_plus_id(name, pid);
	if (!handle)
		hlog("Failed to open mutex '%s': %lu", name, GetLastError());
	return handle;
}

static inline bool init_signals(void)
{
	// create event signal handle
	DWORD pid = GetCurrentProcessId();

	signal_restart = init_event(EVENT_CAPTURE_RESTART, pid);
	if (!signal_restart) {
		return false;
	}

	signal_alive = init_event(EVENT_HOOK_TARGETALIVE, pid);
	if (!signal_alive) {
		return false;
	}

	signal_stop = init_event(EVENT_CAPTURE_STOP, pid);
	if (!signal_stop) {
		return false;
	}

	signal_ready = init_event(EVENT_HOOK_READY, pid);
	if (!signal_ready) {
		return false;
	}

	signal_exit = init_event(EVENT_HOOK_EXIT, pid);
	if (!signal_exit) {
		return false;
	}

	signal_init = init_event(EVENT_HOOK_INIT, pid);
	if (!signal_init) {
		return false;
	}

	return true;
}

static inline bool init_mutexes(void)
{
	DWORD pid = GetCurrentProcessId();

	tex_mutexes[0] = init_mutex(MUTEX_TEXTURE1, pid);
	if (!tex_mutexes[0]) {
		return false;
	}

	tex_mutexes[1] = init_mutex(MUTEX_TEXTURE2, pid);
	if (!tex_mutexes[1]) {
		return false;
	}

	return true;
}

static inline bool init_system_path(void)
{
	UINT ret = GetSystemDirectoryA(system_path, MAX_PATH);
	if (!ret) {
		hlog("Failed to get windows system path: %lu", GetLastError());
		return false;
	}

	return true;
}

static inline void log_current_process(void)
{
	DWORD len = GetModuleBaseNameA(GetCurrentProcess(), NULL, process_name,
				       MAX_PATH);
	if (len > 0) {
		process_name[len] = 0;
		hlog("Hooked to process: %s", process_name);
	}

	hlog("(half life scientist) everything..  seems to be in order");
}

#include <inttypes.h>
static inline bool init_hook_info(void)
{
	hlog("[NCCAPTURER::hook.c!::init_hook_info] =============================INIT_HOOK_INFO===============================");

	filemap_hook_info = create_hook_info(GetCurrentProcessId());
	if (!filemap_hook_info) {
		hlog("Failed to create hook info file mapping: %lu",
		     GetLastError());
		return false;
	}

	global_hook_info = MapViewOfFile(filemap_hook_info, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(struct hook_info));
	if (!global_hook_info) 
	{
		hlog("Failed to map the hook info file mapping: %lu",
		     GetLastError());
		return false;
	}

	set_hook_graphics_support(global_hook_info, GRAPHICS_SUPPORT_ALL);

	return true;
}

#define DEF_FLAGS (WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS)

static DWORD WINAPI dummy_window_thread(LPVOID *unused)
{
	static const wchar_t dummy_window_class[] = L"temp_d3d_window_4039785";
	WNDCLASSW wc;
	MSG msg;

	memset(&wc, 0, sizeof(wc));
	wc.style = CS_OWNDC;
	wc.hInstance = dll_inst;
	wc.lpfnWndProc = (WNDPROC)DefWindowProc;
	wc.lpszClassName = dummy_window_class;

	if (!RegisterClass(&wc)) {
		hlog("Failed to create temp D3D window class: %lu",
		     GetLastError());
		return 0;
	}

	dummy_window = CreateWindowExW(0, dummy_window_class, L"Temp Window",
				       DEF_FLAGS, 0, 0, 1, 1, NULL, NULL,
				       dll_inst, NULL);
	if (!dummy_window) {
		hlog("Failed to create temp D3D window: %lu", GetLastError());
		return 0;
	}

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	(void)unused;
	return 0;
}

static inline void init_dummy_window_thread(void)
{
	HANDLE thread =
		CreateThread(NULL, 0, dummy_window_thread, NULL, 0, NULL);
	if (!thread) {
		hlog("Failed to create temp D3D window thread: %lu",
		     GetLastError());
		return;
	}

	CloseHandle(thread);
}

static inline bool init_hook(HANDLE thread_handle)
{
	hlog("[NCCAPTURER::hook.c!::init_hook] thread_handle(0x%x)", thread_handle);

	wait_for_dll_main_finish(thread_handle);

	_snwprintf(keepalive_name, sizeof(keepalive_name) / sizeof(wchar_t),
		   L"%s%lu", WINDOW_HOOK_KEEPALIVE, GetCurrentProcessId());

	init_pipe();

	init_dummy_window_thread();
	log_current_process();

	hlog("[NCCAPTURER::hook.c!::init_hook] SetEvent(hook_restart)");

	SetEvent(signal_restart);

	hlog("[NCCAPTURER::hook.c!::init_hook] finish");

	return true;
}

static inline void close_handle(HANDLE *handle)
{
	if (*handle) {
		CloseHandle(*handle);
		*handle = NULL;
	}
}

static void free_hook(void)
{
	if (filemap_hook_info) {
		CloseHandle(filemap_hook_info);
		filemap_hook_info = NULL;
	}
	if (global_hook_info) {
		UnmapViewOfFile(global_hook_info);
		global_hook_info = NULL;
	}

	close_handle(&tex_mutexes[1]);
	close_handle(&tex_mutexes[0]);
	close_handle(&signal_exit);
	close_handle(&signal_ready);
	close_handle(&signal_stop);
	close_handle(&signal_restart);
	// TODO
//	ipc_pipe_client_free(&pipe);
}

static inline bool d3d8_hookable(void)
{
	return !!global_hook_info->offsets.d3d8.present;
}

static inline bool ddraw_hookable(void)
{
	return !!global_hook_info->offsets.ddraw.surface_create &&
	       !!global_hook_info->offsets.ddraw.surface_restore &&
	       !!global_hook_info->offsets.ddraw.surface_release &&
	       !!global_hook_info->offsets.ddraw.surface_unlock &&
	       !!global_hook_info->offsets.ddraw.surface_blt &&
	       !!global_hook_info->offsets.ddraw.surface_flip &&
	       !!global_hook_info->offsets.ddraw.surface_set_palette &&
	       !!global_hook_info->offsets.ddraw.palette_set_entries;
}

static inline bool d3d9_hookable(void)
{
	return !!global_hook_info->offsets.d3d9.present &&
	       !!global_hook_info->offsets.d3d9.present_ex &&
	       !!global_hook_info->offsets.d3d9.present_swap;
}

static inline bool dxgi_hookable(void)
{
	return !!global_hook_info->offsets.dxgi.present &&
	       !!global_hook_info->offsets.dxgi.resize;
}

//static bool ddraw_hooked = false;
static bool d3d8_hooked = false;
static bool d3d9_hooked = false;
static bool dxgi_hooked = false;
static bool gl_hooked = false;
static inline bool attempt_hook(void)
{
	hlog("[NCCAPTURER::hook.c!::attempt_hook] ==================================ATEMPT_HOOK===============================");

	bool res = false;

	if (is_hook_graphics_support_d3d9(global_hook_info->support))
	{
		if (!d3d9_hooked) 
		{
			if (!d3d9_hookable()) 
			{
				hlog("[NCCAPTURER::hook.c!::attempt_hook] no D3D9 hook address found!");
				d3d9_hooked = true;
			} 
			else 
			{
				d3d9_hooked = hook_d3d9();
				if (d3d9_hooked) 
				{
					hlog("[NCCAPTURER::hook.c!::attempt_hook] D3D9 hooked!!");
					res = true;
				}
			}
		}
	}

	if (is_hook_graphics_support_dxgi(global_hook_info->support))
	{
		if (!dxgi_hooked) 
		{
			if (!dxgi_hookable()) 
			{
				hlog("[NCCAPTURER::hook.c!::attempt_hook] no DXGI hook address found!");
				dxgi_hooked = true;
			} 
			else 
			{
				dxgi_hooked = hook_dxgi();
				if (dxgi_hooked) 
				{
					hlog("[NCCAPTURER::hook.c!::attempt_hook] DXGI hooked!!");
					res = true;
				}
			}
		}
	}

	if (is_hook_graphics_support_opengl(global_hook_info->support))
	{
		if (!gl_hooked) 
		{
			gl_hooked = hook_gl();
			if (gl_hooked) 
			{
				hlog("[NCCAPTURER::hook.c!::attempt_hook] GL hooked!!");
				res = true;
			}
			/*} else {
			rehook_gl();*/
		}
	}

	if (is_hook_graphics_support_d3d8(global_hook_info->support))
	{
		if (!d3d8_hooked) 
		{
			if (!d3d8_hookable()) 
			{
				d3d8_hooked = true;
			} 
			else 
			{
				d3d8_hooked = hook_d3d8();
				if (d3d8_hooked) 
				{
					hlog("[NCCAPTURER::hook.c!::attempt_hook] D3D8 hooked!!");
					res = true;
				}
			}
		}
	}

	if (is_hook_graphics_support_ddraw(global_hook_info->support))
	{
		/*if (!ddraw_hooked) {
			if (!ddraw_hookable()) {
				ddraw_hooked = true;
			} else {
				ddraw_hooked = hook_ddraw();
				if (ddraw_hooked) {
					hlog("[NCCAPTURER::hook.c!::attempt_hook] DDRAW hooked!!");
					return true;
				}
			}
		}*/
	}

	return res;
}

static inline bool retry_hook(void)
{
	unhook_dxgi();
	unhook_gl();
	dxgi_hooked = false;
	gl_hooked = false;
	return attempt_hook();
}

static inline void capture_loop(void)
{
	WaitForSingleObject(signal_init, INFINITE);

	while (!attempt_hook())
		Sleep(40);

	set_hook_state(HOOK_STATE_HOOKED);

	for (size_t n = 0; !stop_loop; n++) {
		/* this causes it to check every 4 seconds, but still with
		 * a small sleep interval in case the thread needs to stop */
		if (n % 100 == 0)
			attempt_hook();
		Sleep(40);

#if USE_CAPTURER_DETUOUR_RECOVER
		// OBS 후킹(no-trampoline 방식 후킹)과 중복하여 후킹을 위한 재시도
		if (!capture_active())
		{
			static _retry_hook_count = 0;
			if (_retry_hook_count<DETOUR_HOOKING_MAX_RETRY_COUNT)
			{	
				hlog("[NCCAPTURER::hook.c!::capture_loop] check retry hooking count(%d)", ++_retry_hook_count);
				retry_hook();
			}
		}
#endif
	}
}

bool get_offsets()
{
	hlog("[NCCAPTURER::hook.c!::get_offsets]");

	// get offsets
	struct graphics_offsets offsets = { 0 };
	
	WNDCLASSA wc = { 0 };
	wc.style = CS_OWNDC;
	wc.hInstance = GetModuleHandleA(NULL);
	wc.lpfnWndProc = (WNDPROC)DefWindowProcA;
	wc.lpszClassName = DUMMY_WNDCLASS;

	SetErrorMode(SEM_FAILCRITICALERRORS);

	if (!RegisterClassA(&wc))
	{
		hlog("failed to register '%s'", DUMMY_WNDCLASS);
		hlog("[NCCAPTURER::hook.c!::get_offsets] offsets.d3d8.present(0x%x)", global_hook_info->offsets.d3d8.present);
		return false;
	}

	get_d3d9_offsets(&offsets.d3d9);
	get_d3d8_offsets(&offsets.d3d8);
	get_dxgi_offsets(&offsets.dxgi);

	global_hook_info->offsets = offsets;

	hlog("[NCCAPTURER::hook.c!::get_offsets] offsets.d3d8.present(0x%x)", global_hook_info->offsets.d3d8.present);
	hlog("[NCCAPTURER::hook.c!::get_offsets] offsets.d3d9.present(0x%x)", global_hook_info->offsets.d3d9.present);
	hlog("[NCCAPTURER::hook.c!::get_offsets] offsets.d3d9.present_ex(0x%x)", global_hook_info->offsets.d3d9.present_ex);
	hlog("[NCCAPTURER::hook.c!::get_offsets] offsets.d3d9.present_swap(0x%x)", global_hook_info->offsets.d3d9.present_swap);
	hlog("[NCCAPTURER::hook.c!::get_offsets] offsets.d3d9.d3d9_clsoff(0x%x)", global_hook_info->offsets.d3d9.d3d9_clsoff);
	hlog("[NCCAPTURER::hook.c!::get_offsets] offsets.d3d9.is_d3d9ex_clsoff(0x%x)", global_hook_info->offsets.d3d9.is_d3d9ex_clsoff);
	hlog("[NCCAPTURER::hook.c!::get_offsets] offsets.dxgi.present(0x%x)", global_hook_info->offsets.dxgi.present);
	hlog("[NCCAPTURER::hook.c!::get_offsets] offsets.dxgi.present1(0x%x)", global_hook_info->offsets.dxgi.present1);
	hlog("[NCCAPTURER::hook.c!::get_offsets] offsets.dxgi.resize(0x%x)", global_hook_info->offsets.dxgi.resize);

	return true;
}

static DWORD WINAPI main_capture_thread(HANDLE thread_handle)
{
	hlog("[NCCAPTURER::hook.c!::main_capture_thread]");	

	if(!get_offsets()) {
		hlog("[NCCAPTURER::hook.c!::main_capture_thread] Failed to get_offsets");	
		return 0;
	}

	if (!init_hook(thread_handle)) {
		hlog("[NCCAPTURER::hook.c!::main_capture_thread] Failed to init_hook");
		free_hook();
		return 0;
	}

	capture_loop();

	hlog("[NCCAPTURER::hook.c!::main_capture_thread] finish");	
	return 0;
}

static inline uint64_t get_clockfreq(void)
{
	static bool have_clockfreq = false;
	static LARGE_INTEGER clock_freq;

	if (!have_clockfreq) {
		QueryPerformanceFrequency(&clock_freq);
		have_clockfreq = true;
	}

	return clock_freq.QuadPart;
}

uint64_t os_gettime_ns(void)
{
	LARGE_INTEGER current_time;
	double time_val;

	QueryPerformanceCounter(&current_time);
	time_val = (double)current_time.QuadPart;
	time_val *= 1000000000.0;
	time_val /= (double)get_clockfreq();

	return (uint64_t)time_val;
}

static inline int try_lock_shmem_tex(int id)
{
	int next = id == 0 ? 1 : 0;
	DWORD wait_result = WAIT_FAILED;

	wait_result = WaitForSingleObject(tex_mutexes[id], 0);
	if (wait_result == WAIT_OBJECT_0 || wait_result == WAIT_ABANDONED) {
		return id;
	}

	wait_result = WaitForSingleObject(tex_mutexes[next], 0);
	if (wait_result == WAIT_OBJECT_0 || wait_result == WAIT_ABANDONED) {
		return next;
	}

	return -1;
}

static inline void unlock_shmem_tex(int id)
{
	if (id != -1) {
		ReleaseMutex(tex_mutexes[id]);
	}
}

static inline bool init_shared_info(size_t size)
{
	wchar_t name[64];
	_snwprintf(name, 64, L"%s%u", SHMEM_TEXTURE, shmem_id);
	hlog("[NCCAPTURER::hook.c!::init_shared_info] shmem_id(%d)", shmem_id);

	shmem_file_handle = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD)size, name);
	if (!shmem_file_handle) {
		hlog("init_shared_info: Failed to create shared memory: %d", GetLastError());
		return false;
	}

	shmem_info = MapViewOfFile(shmem_file_handle, FILE_MAP_ALL_ACCESS, 0, 0, size);
	if (!shmem_info) {
		hlog("init_shared_info: Failed to map shared memory: %d", GetLastError());
		return false;
	}

	return true;
}

bool capture_init_shtex(struct shtex_data **data, HWND window, uint32_t base_cx,
			uint32_t base_cy, uint32_t cx, uint32_t cy,
			uint32_t format, bool flip, uintptr_t handle)
{
	shmem_id = (uint32_t)(uintptr_t)window;
	if (!init_shared_info(sizeof(struct shtex_data))) {
		hlog("[NCCAPTURER::hook.c!::capture_init_shtex] [error] Failed to initialize memory");
		return false;
	}

	*data = shmem_info;
	(*data)->tex_handle = (uint32_t)handle;

	hlog("[NCCAPTURER::hook.c!::capture_init_shtex] shtex_info(0x%p) tex_handle(0x%p)", *data, handle);

	global_hook_info->window = (uint32_t)(uintptr_t)window;
	global_hook_info->type = CAPTURE_TYPE_TEXTURE;
	global_hook_info->format = format;
	global_hook_info->flip = flip;
	global_hook_info->map_id = shmem_id;
	global_hook_info->map_size = sizeof(struct shtex_data);
	global_hook_info->cx = cx;
	global_hook_info->cy = cy;
	global_hook_info->base_cx = base_cx;
	global_hook_info->base_cy = base_cy;

	update_hook_ready_key(global_hook_info);
	set_hook_state(HOOK_STATE_ACTIVE);

	active = true;
	hlog("[NCCAPTURER::hook.c!::capture_init_shtex] active(%d)", active);

	return true;
}

static DWORD CALLBACK copy_thread(LPVOID unused)
{
	uint32_t pitch = thread_data.pitch;
	uint32_t cy = thread_data.cy;
	HANDLE events[2] = {NULL, NULL};
	int shmem_id = 0;

	if (!duplicate_handle(&events[0], thread_data.copy_event)) {
		hlog_hr("copy_thread: Failed to duplicate copy event: %d",
			GetLastError());
		return 0;
	}

	if (!duplicate_handle(&events[1], thread_data.stop_event)) {
		hlog_hr("copy_thread: Failed to duplicate stop event: %d",
			GetLastError());
		goto finish;
	}

	for (;;) {
		int copy_tex;
		void *cur_data;

		DWORD ret = WaitForMultipleObjects(2, events, false, INFINITE);
		if (ret != WAIT_OBJECT_0) {
			break;
		}

		EnterCriticalSection(&thread_data.data_mutex);
		copy_tex = thread_data.cur_tex;
		cur_data = thread_data.cur_data;
		LeaveCriticalSection(&thread_data.data_mutex);

		if (copy_tex < NUM_BUFFERS && !!cur_data) {
			EnterCriticalSection(&thread_data.mutexes[copy_tex]);

			int lock_id = try_lock_shmem_tex(shmem_id);
			if (lock_id != -1) {
				memcpy(thread_data.shmem_textures[lock_id],
				       cur_data, pitch * cy);

				unlock_shmem_tex(lock_id);
				((struct shmem_data *)shmem_info)->last_tex =
					lock_id;

				shmem_id = lock_id == 0 ? 1 : 0;
			}

			LeaveCriticalSection(&thread_data.mutexes[copy_tex]);
		}
	}

finish:
	for (size_t i = 0; i < 2; i++) {
		if (events[i]) {
			CloseHandle(events[i]);
		}
	}

	(void)unused;
	return 0;
}

void shmem_copy_data(size_t idx, void *volatile data)
{
	EnterCriticalSection(&thread_data.data_mutex);
	thread_data.cur_tex = (int)idx;
	thread_data.cur_data = data;
	thread_data.locked_textures[idx] = true;
	LeaveCriticalSection(&thread_data.data_mutex);

	SetEvent(thread_data.copy_event);
}

bool shmem_texture_data_lock(int idx)
{
	bool locked;

	EnterCriticalSection(&thread_data.data_mutex);
	locked = thread_data.locked_textures[idx];
	LeaveCriticalSection(&thread_data.data_mutex);

	if (locked) {
		EnterCriticalSection(&thread_data.mutexes[idx]);
		return true;
	}

	return false;
}

void shmem_texture_data_unlock(int idx)
{
	EnterCriticalSection(&thread_data.data_mutex);
	thread_data.locked_textures[idx] = false;
	LeaveCriticalSection(&thread_data.data_mutex);

	LeaveCriticalSection(&thread_data.mutexes[idx]);
}

static inline bool init_shmem_thread(uint32_t pitch, uint32_t cy)
{
	struct shmem_data *data = shmem_info;

	thread_data.pitch = pitch;
	thread_data.cy = cy;
	thread_data.shmem_textures[0] = (uint8_t *)data + data->tex1_offset;
	thread_data.shmem_textures[1] = (uint8_t *)data + data->tex2_offset;

	thread_data.copy_event = CreateEvent(NULL, false, false, NULL);
	if (!thread_data.copy_event) {
		hlog("init_shmem_thread: Failed to create copy event: %d",
		     GetLastError());
		return false;
	}

	thread_data.stop_event = CreateEvent(NULL, true, false, NULL);
	if (!thread_data.stop_event) {
		hlog("init_shmem_thread: Failed to create stop event: %d",
		     GetLastError());
		return false;
	}

	for (size_t i = 0; i < NUM_BUFFERS; i++) {
		InitializeCriticalSection(&thread_data.mutexes[i]);
	}

	InitializeCriticalSection(&thread_data.data_mutex);

	thread_data.copy_thread =
		CreateThread(NULL, 0, copy_thread, NULL, 0, NULL);
	if (!thread_data.copy_thread) {
		hlog("init_shmem_thread: Failed to create thread: %d",
		     GetLastError());
		return false;
	}
	return true;
}

#ifndef ALIGN
#define ALIGN(bytes, align) (((bytes) + ((align)-1)) & ~((align)-1))
#endif

bool capture_init_shmem(struct shmem_data **data, HWND window, uint32_t base_cx,
			uint32_t base_cy, uint32_t cx, uint32_t cy,
			uint32_t pitch, uint32_t format, bool flip)
{
	uint32_t tex_size = cy * pitch;
	uint32_t aligned_header = ALIGN(sizeof(struct shmem_data), 32);
	uint32_t aligned_tex = ALIGN(tex_size, 32);
	uint32_t total_size = aligned_header + aligned_tex * 2 + 32;
	uintptr_t align_pos;

	shmem_id = (uint32_t)(uintptr_t)window;
	if (!init_shared_info(total_size)) {
		hlog("capture_init_shmem: Failed to initialize memory");
		return false;
	}

	*data = shmem_info;

	/* to ensure fast copy rate, align texture data to 256bit addresses */
	align_pos = (uintptr_t)shmem_info;
	align_pos += aligned_header;
	align_pos &= ~(32 - 1);
	align_pos -= (uintptr_t)shmem_info;

	if (align_pos < sizeof(struct shmem_data))
		align_pos += 32;

	(*data)->last_tex = -1;
	(*data)->tex1_offset = (uint32_t)align_pos;
	(*data)->tex2_offset = (*data)->tex1_offset + aligned_tex;

	global_hook_info->window = (uint32_t)(uintptr_t)window;
	global_hook_info->type = CAPTURE_TYPE_MEMORY;
	global_hook_info->format = format;
	global_hook_info->flip = flip;
	global_hook_info->map_id = shmem_id;
	global_hook_info->map_size = total_size;
	global_hook_info->pitch = pitch;
	global_hook_info->cx = cx;
	global_hook_info->cy = cy;
	global_hook_info->base_cx = base_cx;
	global_hook_info->base_cy = base_cy;

	if (!init_shmem_thread(pitch, cy)) {
		return false;
	}

	update_hook_ready_key(global_hook_info);
	set_hook_state(HOOK_STATE_ACTIVE);

	active = true;
	hlog("[NCCAPTURER::hook.c!::capture_init_shmem] active(%d)", active);

	return true;
}

static inline void thread_data_free(void)
{
	if (thread_data.copy_thread) {
		DWORD ret;

		SetEvent(thread_data.stop_event);
		ret = WaitForSingleObject(thread_data.copy_thread, 500);
		if (ret != WAIT_OBJECT_0)
			TerminateThread(thread_data.copy_thread, (DWORD)-1);

		CloseHandle(thread_data.copy_thread);
	}
	if (thread_data.stop_event)
		CloseHandle(thread_data.stop_event);
	if (thread_data.copy_event)
		CloseHandle(thread_data.copy_event);
	for (size_t i = 0; i < NUM_BUFFERS; i++)
		DeleteCriticalSection(&thread_data.mutexes[i]);

	DeleteCriticalSection(&thread_data.data_mutex);

	memset(&thread_data, 0, sizeof(thread_data));
}

void capture_free(void)
{
	hlog("[NCCAPTURER::hook.c!::capture_free]");

	thread_data_free();

	if (shmem_info) {
		UnmapViewOfFile(shmem_info);
		shmem_info = NULL;
	}

	close_handle(&shmem_file_handle);

	set_hook_state(HOOK_STATE_HOOKED);

	hlog("[NCCAPTURER::hook.c!::capture_free] SetEvent(signal_restart)");
	SetEvent(signal_restart);

	active = false;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID unused1)
{
	if (reason == DLL_PROCESS_ATTACH) 
	{
		hlog("[NCCAPTURER::hook.c!::DllMain] DLL_PROCESS_ATTACH");

		wchar_t name[MAX_PATH];

		dll_inst = hinst;

		HANDLE cur_thread;
		bool success = DuplicateHandle(GetCurrentProcess(),
					       GetCurrentThread(),
					       GetCurrentProcess(), &cur_thread,
					       SYNCHRONIZE, false, 0);

		if (!success)
			hlog("[NCCAPTURER::hook.c!::DllMain] Failed to get current thread handle");
		else
			hlog("[NCCAPTURER::hook.c!::DllMain] get current thread handle OK!!!");

		if (!init_signals()) {
			return false;
		}
		if (!init_system_path()) {
			return false;
		}
		if (!init_hook_info()) {
			return false;
		}
		if (!init_mutexes()) {
			return false;
		}

		set_hook_state(HOOK_STATE_INJECTED);

		/* this prevents the library from being automatically unloaded
		 * by the next FreeLibrary call */
		GetModuleFileNameW(hinst, name, MAX_PATH);
		LoadLibraryW(name);

		capture_thread = CreateThread(
			NULL, 0, (LPTHREAD_START_ROUTINE)main_capture_thread,
			(LPVOID)cur_thread, 0, 0);
		if (!capture_thread) {
			CloseHandle(cur_thread);
			return false;
		}

	} else if (reason == DLL_PROCESS_DETACH) 
	{
		hlog("[NCCAPTURER::hook.c!::DllMain] DLL_PROCESS_DETACH");

		if (capture_thread) 
		{
			stop_loop = true;
			WaitForSingleObject(capture_thread, 300);
			CloseHandle(capture_thread);
		}

		free_hook();
	}

	(void)unused1;
	return true;
}

__declspec(dllexport) LRESULT CALLBACK
	dummy_debug_proc(int code, WPARAM wparam, LPARAM lparam)
{
	static bool hooking = true;
	MSG *msg = (MSG *)lparam;

	if (hooking && msg->message == (WM_USER + 432)) {
		HMODULE user32 = GetModuleHandleW(L"USER32");
		BOOL(WINAPI * unhook_windows_hook_ex)(HHOOK) = NULL;

		unhook_windows_hook_ex = get_encrypted_func(
			user32, "VojeleY`bdgxvM`hhDz", 0x7F55F80C9EE3A213ULL);

		if (unhook_windows_hook_ex)
			unhook_windows_hook_ex((HHOOK)msg->lParam);
		hooking = false;
	}

	return CallNextHookEx(0, code, wparam, lparam);
}
