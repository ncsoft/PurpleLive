#include "pch.h"
#include "NCCapture.h"
#include "hook-helpers.h"
#include "graphics-hook-info.h"
#include "enc.h"
#include <psapi.h>
#include <dxgiformat.h>
#include "Timer.h"
#include "LockObject.h"
#include "VideoDevice.h"
#include "DirectXTex.h"
#include "StructureDefines.h"
#include "Texture2D.h"
#include "easylogging++.h"

static void close_handle(HANDLE *p_handle);

Time g_timer;
std::mutex mtx_lock;

NCCapture::NCCapture()
{

}

NCCapture::~NCCapture()
{
	LOG(VERBOSE) << format_string("[NCCapture::~NCCapture]");

	StopCapture();

	if (m_pGC)
	{
		delete m_pGC;
		m_pGC = nullptr;
	}

	LOG(INFO) << format_string("[NCCapture::~NCCapture] Singleton");
}

void NCCapture::InitLog(const char* path)
{
	elpp_set_path(path);
}

void NCCapture::SetEnableLog(bool error, bool info, bool debug)
{
	elpp_set_enable_log(error, info, debug);
}

bool NCCapture::IsStartedCapture()
{
	return (m_pGC);
}

bool NCCapture::IsReadyCapture()
{
	return m_pGC && m_pGC->is_hooked();
}

bool NCCapture::IsCapturing()
{
	return m_pGC && m_pGC->capturing;
}

bool NCCapture::StartCapture()
{
	if (m_hwnd==nullptr)
		return false;

	return StartCapture(m_hwnd, m_record);
}

void NCCapture::RegisterDeviceCreatedCallback(DEVICE_CREATED_CALLBACK_FUNC callback, void* user_data)
{
	VideoDevice::Instance()->RegisterCreatedCallback(callback, user_data);
}

bool NCCapture::StartCapture(const char* title, bool record)
{
	m_pGC->strTitle = title;
	HWND hwnd = get_selected_window();

	return StartCapture(hwnd, record);
}

bool NCCapture::StartCapture(HWND hwnd, bool record)
{
	if (m_pGC)
		return false;

	m_hwnd = hwnd;
	m_record = record;

	ready_capture();

	m_pGC->window = hwnd;
	m_pGC->activate_hook = true;
	g_timer.reset();
	LOG(VERBOSE) << format_string("[NCCapture::StartCapture] CreateThread()");
	m_b_update_thread = true;
	m_h_update_thread = CreateThread(nullptr, 0, ThreadProc, this, 0, nullptr);

	if (m_h_update_thread == NULL)
		return false;

	return true;
}

void NCCapture::StopCapture()
{
	if (m_h_update_thread)
	{
		m_b_update_thread = false;
		WaitForSingleObject(m_h_update_thread, INFINITE);
		CloseHandle(m_h_update_thread);
		m_h_update_thread = 0;
	}

	if (m_pGC)
	{
		m_pGC->activate_hook = false;
		stop_capture();
		m_pGC = nullptr;
	}
}

bool NCCapture::CreateVideoDevice(HWND hwnd, unsigned int width, unsigned int height)
{
	return VideoDevice::Instance()->Create(hwnd, width, height);
}

void NCCapture::ResizeVideoDevice(int width, int height)
{
	VideoDevice::Instance()->Resize(width, height);
}

void NCCapture::Update()
{
	VideoDevice::Instance()->Update();
}

void NCCapture::DrawRect(const FRECT& dest, uint32_t color)
{
	VideoDevice::Instance()->DrawRect(dest, color);
}

void NCCapture::DrawRectInGame(const SIZE & src, const FRECT & dest, uint32_t color)
{
	VideoDevice::Instance()->DrawRectInGame(src, dest, color);
}

void NCCapture::BeginRender()
{
	VideoDevice::Instance()->BeginRender();
}

void NCCapture::EndRender()
{
	VideoDevice::Instance()->EndRender();
}

void NCCapture::OnActivated()
{
	VideoDevice::Instance()->OnActivated();
}

void NCCapture::OnDeactivated()
{
	VideoDevice::Instance()->OnDeactivated();
}

void NCCapture::OnSuspending()
{
	VideoDevice::Instance()->OnSuspending();
}

void NCCapture::OnResuming()
{
	VideoDevice::Instance()->OnResuming();
}

void NCCapture::OnResizing(int width, int height)
{
	VideoDevice::Instance()->OnResizing(width, height);
}

const CapturedImage* NCCapture::GetFrameBufferImage()
{
	return VideoDevice::Instance()->GetFrameBufferImage();
}

/*
CapturedImage* NCCapture::GetCapturedImage()
{
	if (m_pGC == nullptr)
		return nullptr;
	if (m_pGC->texture == nullptr)
		return nullptr;

	const DirectX::Image* pImage = m_pGC->texture->GetImage()->GetImages();
	if (pImage == nullptr)
		return nullptr;

	m_captured_image.width = pImage->width;
	m_captured_image.height = pImage->height;
	m_captured_image.rowPitch = pImage->rowPitch;
	m_captured_image.slicePitch = pImage->slicePitch;
	m_captured_image.pixels = pImage->pixels;

	return &m_captured_image;
}
*/
void* NCCapture::GetCapturedTexture()
{
	if (m_pGC->texture == nullptr)
		return nullptr;

	return (void*)m_pGC->texture->GetTexture();
}

void* NCCapture::GetCapturedSRV(int& width, int& height, bool& flip)
{
	std::lock_guard<std::mutex> guard(g_mtx_lock_video);
	if (m_pGC == nullptr)
		return nullptr;
	if (m_pGC->texture == nullptr)
		return nullptr;

	width = m_pGC->texture->GetWidth();
	height = m_pGC->texture->GetHeight();
	flip = m_pGC->texture->GetFlip();

	return (void*)m_pGC->texture->GetSRV();
}

void* NCCapture::GetCapturedSRV(int& width, int& height, bool& flip, int& hook_ready_key)
{
	std::lock_guard<std::mutex> guard(g_mtx_lock_video);
	if (m_pGC == nullptr)
		return nullptr;
	if (m_pGC->texture == nullptr)
		return nullptr;

	width = m_pGC->texture->GetWidth();
	height = m_pGC->texture->GetHeight();
	flip = m_pGC->texture->GetFlip();
	hook_ready_key = m_pGC->hook_ready_key;

	return (void*)m_pGC->texture->GetSRV();
}

void NCCapture::try_hook()
{
	LOG(DEBUG) << format_string("[NCCapture::try_hook] =======Try Hook Start=======");
	
	if (!m_pGC->window)
		return;

	m_pGC->thread_id = GetWindowThreadProcessId(m_pGC->window, &m_pGC->process_id);

	if (m_pGC->thread_id == 0)
	{
		LOG(ERROR) << format_string("[NCCapture::try_hook] GetWindowThreadProcessId Failed!!");
	}
	else
	{
		LOG(DEBUG) << format_string("[NCCapture::try_hook] GetWindowThreadProcessId OK!!");
	}

	if (m_pGC->process_id == GetCurrentProcessId())
		return;

	if (!m_pGC->thread_id && m_pGC->process_id)
		return;

	if (!m_pGC->process_id)
	{
		return;
	}

	// init_hook
	if (init_hook() == false)
	{
		LOG(ERROR) << format_string("[NCCapture::try_hook] init_hook failed");
		stop_capture();
	}

	LOG(VERBOSE) << format_string("[NCCapture::try_hook] =======Try Hook End=======");
}

void NCCapture::try_start_capture()
{
	eCaptureResult re = init_capture_data();

	if (re == eCR_SUCCESS)
	{
		m_pGC->capturing = start_capture();
	}
	else
	{
		LOG(ERROR) << format_string("[NCCapture::try_start_capture] failed init_capture_data");
	}

	if (re != eCR_RETRY && !m_pGC->capturing) 
	{
		m_pGC->retry_interval = DEFAULT_RETRY_INTERVAL;
		stop_capture();
	}

	// hook_ready_key 업데이트
	m_pGC->hook_ready_key = get_hook_ready_key(m_pGC->global_hook_info);
}

DWORD WINAPI NCCapture::ThreadProc(LPVOID p)
{
	NCCapture* capture = (NCCapture*)p;
	while (capture->_is_active_update_thread())
	{
		capture->update();
	}
	return 0;
}

void NCCapture::update()
{
	if (m_pGC == nullptr)
		return;

	float delta = g_timer.update();

	// remove hook_stop event handling

	if (m_pGC->active && !m_pGC->hook_ready && m_pGC->process_id)
	{
		LOG(DEBUG) << format_string("[NCCapture::update] hook_ready open event");
		m_pGC->hook_ready = _open_event(EVENT_HOOK_READY);
	}

	if (m_pGC->injector_process && object_signalled(m_pGC->injector_process))
	{
		DWORD exit_code = 0;
		GetExitCodeProcess(m_pGC->injector_process, &exit_code);
		close_handle(&m_pGC->injector_process);
		LOG(DEBUG) << format_string("[NCCapture::update] Close inject process");
		if (exit_code != 0)
		{
			LOG(ERROR) << format_string("[NCCapture::update] inject process failed: %ld", (long)exit_code);
			m_pGC->error_acquiring = true;
		}
		else if (!m_pGC->capturing)
		{
			LOG(DEBUG) << format_string("[NCCapture::update] stop capture in close injector process");
			m_pGC->retry_interval = DEFAULT_RETRY_INTERVAL;
			stop_capture();
		}
	}

	if (m_pGC->hook_ready_key != get_hook_ready_key(m_pGC->global_hook_info))
	{
		try_start_capture();
	}

	if (!m_pGC->capturing && m_pGC->is_hooked()) 
	{
		try_start_capture();
	}

	// add delta time
	m_pGC->retry_time += delta;

	if (m_pGC->active)
	{
		request_target_alive();

		if (m_pGC->copy_texture)
			m_pGC->copy_texture(m_pGC);
//		if (object_signalled(m_pGC->target_process))
		{
			
		}
	}
	else
	{
//		LOG(DEBUG) << format_string("[NCCapture::update] e: %d, retry_time :%f, interval : %f", m_pGC->error_acquiring, m_pGC->retry_time, m_pGC->retry_interval);
		if (!m_pGC->error_acquiring && m_pGC->retry_time > m_pGC->retry_interval)
		{
			if (m_pGC->activate_hook)
			{
				// try_hook
				try_hook();
				m_pGC->retry_time = 0.0f;
			}
		}

	}

	if (!m_pGC->showing)
		m_pGC->showing = true;
}

bool NCCapture::init_hook()
{
	// 타겟 실행화일명으로 blacklist에 포함되는지 검사
/*	HANDLE process = m_open_process_proc(PROCESS_QUERY_LIMITED_INFORMATION, false, m_pGC->process_id);
	if (!process)
	{
		CloseHandle(process);
		return false;
	}

	wchar_t wname[MAX_PATH];
	if (!GetProcessImageFileNameW(process, wname, MAX_PATH))
	{
		CloseHandle(process);
		return false;
	}

	CloseHandle(process);

	wname;
*/
	if (target_suspended()) 
	{
		LOG(ERROR) << format_string("[NCCapture::init_hook] target_suspended() failed");
		return false;
	}
		

	if (!open_target_process()) 
	{
		LOG(ERROR) << format_string("[NCCapture::init_hook] open_target_process() failed");
		return false;
	}
	
	
	if (!init_keepalive()) 
	{
		LOG(ERROR) << format_string("[NCCapture::init_hook] init_keepalive() failed");
		return false;
	}

	//	if (!init_pipe())
	//		return false;

	bool hook_exist = attempt_existing_hook();
	if (!hook_exist)
	{
		if (!inject_hook())
		{
			LOG(ERROR) << format_string("[NCCapture::init_hook] inject_hook() failed");
			return false;
		}
	}

	if (!init_texture_mutexes()) 
	{
		LOG(ERROR) << format_string("[NCCapture::init_hook] init_texture_mutexes() failed");
		return false;
	}

	if (!init_hook_info()) 
	{
		LOG(DEBUG) << format_string("[NCCapture::init_hook] init_hook_info() failed");
		return false;
	}

	if (!init_events()) 
	{
		LOG(DEBUG) << format_string("[NCCapture::init_hook] init_events() failed");
		return false;
	}

	// <TODO> 게임에 따라 별도의 Graphics 지원 제한 설정
	//		ex) L2M : GRAPHICS_SUPPORT_DXGI
	//		ex) L1M : GRAPHICS_SUPPORT_OPENGL
	set_hook_graphics_support(m_pGC->global_hook_info, GRAPHICS_SUPPORT_DXGI | GRAPHICS_SUPPORT_OPENGL);

	request_target_alive();

	LOG(DEBUG) << format_string("[NCCapture::init_hook] SetEvent(hook_init)");
	SetEvent(m_pGC->hook_init);

	m_pGC->window;
	m_pGC->active = true;
	m_pGC->retrying = 0;

	LOG(INFO) << format_string("[NCCapture::init_hook] success");

	return true;
}

static void close_handle(HANDLE *p_handle)
{
	HANDLE handle = *p_handle;
	if (handle) {
		if (handle != INVALID_HANDLE_VALUE)
			CloseHandle(handle);
		*p_handle = NULL;
	}
}

static inline bool is_16bit_format(uint32_t format)
{
	return format == DXGI_FORMAT_B5G5R5A1_UNORM || format == DXGI_FORMAT_B5G6R5_UNORM;
}

static inline eColorFormat convert_format(uint32_t format)
{
	switch (format) {
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		return eCF_RGBA;
	case DXGI_FORMAT_B8G8R8X8_UNORM:
		return eCF_BGRX;
	case DXGI_FORMAT_B8G8R8A8_UNORM:
		return eCF_BGRA;
	case DXGI_FORMAT_R10G10B10A2_UNORM:
		return eCF_R10G10B10A2;
	case DXGI_FORMAT_R16G16B16A16_UNORM:
		return eCF_RGBA16;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		return eCF_RGBA16F;
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		return eCF_RGBA32F;
	}

	return eCF_UNKNOWN;
}

static void copy_shmem_tex(GameCapture* gc)
{
	LOG(DEBUG) << format_string("[NCCapture::copy_shmem_tex]");
	int cur_texture;
	HANDLE mutex = NULL;
//	uint32_t pitch;
	int next_texture;
//	uint8_t *data;

	if (!gc->shmem_data)
		return;

	cur_texture = gc->shmem_data->last_tex;

	if (cur_texture < 0 || cur_texture > 1)
		return;

	next_texture = cur_texture == 1 ? 0 : 1;

	if (object_signalled(gc->texture_mutexes[cur_texture])) {
		mutex = gc->texture_mutexes[cur_texture];

	}
	else if (object_signalled(gc->texture_mutexes[next_texture])) {
		mutex = gc->texture_mutexes[next_texture];
		cur_texture = next_texture;

	}
	else {
		return;
	}

	std::lock_guard<std::mutex> guard(mtx_lock);

	// copy image bits
	// TODO
/*	Image src;
	src.width = gc->cx;
	src.height = gc->cy;
	src.format = (DXGI_FORMAT)gc->global_hook_info->format;
	src.rowPitch = gc->pitch;
	src.slicePitch = src.rowPitch * gc->cy;
	src.pixels = gc->texture_buffers[cur_texture];

	Convert(src, DXGI_FORMAT_B8G8R8A8_UNORM, TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, *(ScratchImage*)gc->image);
*/
//	memcpy(gc->buffer, gc->texture_buffers[cur_texture], gc->pitch * gc->cy);

	ReleaseMutex(mutex);
/*
	if (gs_texture_map(gc->texture, &data, &pitch)) {
		if (gc->convert_16bit) {
			copy_16bit_tex(gc, cur_texture, data, pitch);

		}
		else if (pitch == gc->pitch) {
			memcpy(data, gc->texture_buffers[cur_texture], pitch * gc->cy);
		}
		else {
			uint8_t *input = gc->texture_buffers[cur_texture];
			uint32_t best_pitch = pitch < gc->pitch ? pitch
				: gc->pitch;

			for (uint32_t y = 0; y < gc->cy; y++) {
				uint8_t *line_in = input + gc->pitch * y;
				uint8_t *line_out = data + pitch * y;
				memcpy(line_out, line_in, best_pitch);
			}
		}

		gs_texture_unmap(gc->texture);
	}

	ReleaseMutex(mutex);
*/
}

bool NCCapture::start_capture()
{
	if (m_pGC->global_hook_info->type == CAPTURE_TYPE_MEMORY)
	{
		if (!init_shmem_capture())
		{
			LOG(ERROR) << format_string("[NCCapture::start_capture] failed init_shmem_capture");
			return false;
		}
	}
	else
	{
		if (!init_shtex_capture())
		{
			LOG(ERROR) << format_string("[NCCapture::start_capture] failed init_shtex_capture");
			return false;
		}
	}

	LOG(DEBUG) << format_string("[NCCapture::start_capture] success");
	return true;
}

void NCCapture::stop_capture()
{
	LOG(DEBUG) << format_string("[NCCapture::stop_capture]");
	// TODO
//	ipc_pipe_server_free(&gc->pipe);

	// remove SetEvent(hook_stop)

	if (m_pGC->global_hook_info) {
		UnmapViewOfFile(m_pGC->global_hook_info);
		m_pGC->global_hook_info = NULL;
	}
	if (m_pGC->data) 
	{
		UnmapViewOfFile(m_pGC->data);
		m_pGC->data = NULL;
	}

	if (m_pGC->app_sid)
	{
		LocalFree(m_pGC->app_sid);
		m_pGC->app_sid = NULL;
	}

	close_handle(&m_pGC->hook_restart);
	close_handle(&m_pGC->hook_alive);
	close_handle(&m_pGC->hook_ready);
	close_handle(&m_pGC->hook_exit);
	close_handle(&m_pGC->hook_init);
	close_handle(&m_pGC->hook_data_map);
	close_handle(&m_pGC->keepalive_mutex);
	close_handle(&m_pGC->global_hook_info_map);
	close_handle(&m_pGC->target_process);
	close_handle(&m_pGC->texture_mutexes[0]);
	close_handle(&m_pGC->texture_mutexes[1]);

/*	if (m_pGC->texture) 
	{
		// TODO
//		obs_enter_graphics();
//		gs_texture_destroy(gc->texture);
//		obs_leave_graphics();
		m_pGC->texture = NULL;
	}
*/
/*	if (m_pGC->image)
	{
		delete m_pGC->image;
		m_pGC->image = nullptr;
	}
*/
	if (m_pGC->active)
	{
		LOG(INFO) << format_string("[NCCapture::stop_capture] capture stopped");
	}

	m_pGC->copy_texture = NULL;
	m_pGC->wait_for_target_startup = false;
	m_pGC->active = false;
	m_pGC->capturing = false;

	if (m_pGC->retrying)
		m_pGC->retrying--;
}

bool NCCapture::init_hook_info()
{
	m_pGC->global_hook_info_map = _open_hook_info();
	if (!m_pGC->global_hook_info_map)
	{
		LOG(ERROR) << format_string("[NCCapture::init_hook_info] get_hook_info failed: %lu", GetLastError());
		return false;
	}

	m_pGC->global_hook_info = (hook_info*)MapViewOfFile(m_pGC->global_hook_info_map,
											FILE_MAP_ALL_ACCESS, 0, 0,
											sizeof(*m_pGC->global_hook_info));
	if (!m_pGC->global_hook_info) 
	{
		LOG(ERROR) << format_string("[NCCapture::init_hook_info] failed to map data view: %lu", GetLastError());
		return false;
	}

	if (m_pGC->config.force_shmem)
	{
		LOG(DEBUG) << format_string("[NCCapture::init_hook_info] user is forcing shared memory (multi-adapter compatibility mode)");
	}

//	m_pGC->global_hook_info->offsets = m_pGC->process_is_64bit ? offsets64 : offsets32;
	m_pGC->global_hook_info->capture_overlay = m_pGC->config.capture_overlays;
	m_pGC->global_hook_info->force_shmem = false;//true;
	m_pGC->global_hook_info->use_scale = m_pGC->config.force_scaling;
	if (m_pGC->config.scale_cx)
		m_pGC->global_hook_info->cx = m_pGC->config.scale_cx;
	if (m_pGC->config.scale_cy)
		m_pGC->global_hook_info->cy = m_pGC->config.scale_cy;

	// TODO
	//reset_frame_interval(gc);

	// TODO
	// shared_texture_available()
	// opengl : false, d3d : true
/*	obs_enter_graphics();
	if (!gs_shared_texture_available()) {
		warn("init_hook_info: shared texture capture unavailable");
		gc->global_hook_info->force_shmem = true;
	}
	obs_leave_graphics();
*/
	return true;
}

void NCCapture::ready_capture()
{
	if (!m_kernel32_handle)
	{
		m_kernel32_handle = GetModuleHandleW(L"kernel32");
	}
	if (!m_open_process_proc)
	{
		m_open_process_proc = (open_process_proc_t)get_encrypted_func(m_kernel32_handle, "NuagUykjcxr", 0x1B694B59451ULL);
	}

	// create game capture
	m_pGC = new GameCapture();

	init_capture_config();
}

void NCCapture::init_capture_config()
{
	m_pGC->initial_config = true;
	m_pGC->retry_interval = INITIAL_RETRY_INTERVAL;

	// enable to opengl-overlay
	m_pGC->config.capture_overlays = true;
}

void NCCapture::request_target_alive()
{
	if (m_pGC->hook_alive) {
		SetEvent(m_pGC->hook_alive);
	}
}

bool is_app(HANDLE process)
{
	DWORD size_ret;
	DWORD ret = 0;
	HANDLE token;

	if (OpenProcessToken(process, TOKEN_QUERY, &token)) {
		BOOL success = GetTokenInformation(token, TokenIsAppContainer,
			&ret, sizeof(ret),
			&size_ret);
		if (!success) {
			DWORD error = GetLastError();
			int test = 0;
		}

		CloseHandle(token);
	}
	return !!ret;
}

void NCCapture::setup_window(HWND hwnd)
{
	DWORD dwThreadID = GetWindowThreadProcessId(hwnd, &m_pGC->process_id);

	if (dwThreadID == 0)
	{
		LOG(DEBUG) << format_string("[NCCapture::setup_window] GetWindowThreadProcessId() failed %lu", GetLastError());
	}

	if (m_pGC->process_id)
	{
		HANDLE process = m_open_process_proc(PROCESS_QUERY_INFORMATION, false, m_pGC->process_id);
		if (process)
		{
			m_pGC->is_app = is_app(process);
			if (m_pGC->is_app)
			{
				//m_pGC->app_sid = get_app_sid(process);
			}
			CloseHandle(process);
		}
	}


	HANDLE hook_restart = _open_event(EVENT_CAPTURE_RESTART);
	if (hook_restart)
	{
		m_pGC->wait_for_target_startup = false;
		CloseHandle(hook_restart);
	}

	if (m_pGC->wait_for_target_startup)
	{
		// TODO : hook rate
		m_pGC->retry_interval = 3.0f;// *hook_rate_to_float(m_pGC->config.hook_rate);
		m_pGC->wait_for_target_startup = false;
	}
	else
	{
//		m_pGC->next_window = hwnd;
	}
}

HWND NCCapture::get_selected_window()
{
	HWND window;
	// TODO
/*	if (strcmpi(m_pGC->strClass.c_str(), "dwm") == 0) 
	{
		wchar_t class_w[512];
		wsprintf(class_w, L"%s", m_pGC->strClass.c_str());
		window = FindWindowW(class_w, NULL);
	}
	else 
*/	{
		wchar_t title_w[512];
		wsprintf(title_w, L"%s", m_pGC->strTitle.c_str());
		window = FindWindowW(NULL, title_w);
	}

	return window;
/*	if (window) 
	{
		setup_window(window);
	}
	else 
	{
		m_pGC->wait_for_target_startup = true;
	}
*/
}

bool NCCapture::target_suspended()
{
	return false;
}

bool NCCapture::open_target_process()
{
	m_pGC->target_process = m_open_process_proc(PROCESS_QUERY_INFORMATION | SYNCHRONIZE, false, m_pGC->process_id);

	if (!m_pGC->target_process)
	{
		return false;
	}

	m_pGC->process_is_64bit = is_64bit_process(m_pGC->target_process);
/*
	m_pGC->is_app = is_app(m_pGC->target_process);
	if (m_pGC->is_app)
	{
		gc->app_sid = get_app_sid(m_pGC->target_process);
	}
*/
	return true;
}

bool NCCapture::is_64bit_process(HANDLE process)
{
	BOOL x86 = true;
	if (is_64bit_windows()) {
		bool success = !!IsWow64Process(process, &x86);
		if (!success) {
			return false;
		}
	}

	return !x86;
}

bool NCCapture::is_64bit_windows()
{
#ifdef _WIN64
	return true;
#else
	BOOL x86 = false;
	bool success = !!IsWow64Process(GetCurrentProcess(), &x86);
	return success && !!x86;
#endif
}

bool NCCapture::init_keepalive()
{
	wchar_t new_name[64];
	_snwprintf(new_name, 64, L"%s%lu", WINDOW_HOOK_KEEPALIVE, m_pGC->process_id);

	m_pGC->keepalive_mutex = CreateMutexW(NULL, false, new_name);
	if (!m_pGC->keepalive_mutex) 
	{
//		warn("Failed to create keepalive mutex: %lu", GetLastError());
		return false;
	}

	return true;
}

bool NCCapture::attempt_existing_hook()
{
	LOG(DEBUG) << format_string("[NCCapture::attempt_existing_hook]");
	m_pGC->hook_restart = _open_event(EVENT_CAPTURE_RESTART);
	if (m_pGC->hook_restart) 
	{
		LOG(INFO) << format_string("[NCCapture::attempt_existing_hook] existing hook found, signaling process: (%s)", m_pGC->strExe.c_str());
		SetEvent(m_pGC->hook_restart);
		return true;
	}

	return false;
}

bool NCCapture::inject_hook()
{
	LOG(DEBUG) << format_string("[NCCapture::inject_hook]");
	bool matching_architecture;
	bool success = false;
	const wchar_t* hook_dll;
	const wchar_t* injector;

	std::wstring inject_path;
	std::wstring hook_path;
	
	if (m_pGC->process_is_64bit) 
	{
		hook_dll = L"NCCapturer64.dll";
		injector = L"NCCaptureInjector64.exe";
	}
	else 
	{
		hook_dll = L"NCCapturer32.dll";
		injector = L"NCCaptureInjector32.exe";
	}

	inject_path = get_path() + L"\\" + injector;
	hook_path	= get_path() + L"\\" + hook_dll;

	// TODO injector, capturer 파일 유효성 검사

#ifdef _WIN64
	matching_architecture = m_pGC->process_is_64bit;
#else
	matching_architecture = !m_pGC->process_is_64bit;
#endif

/*	if (matching_architecture && !use_anticheat(gc)) 
	{
		info("using direct hook");
		success = hook_direct(gc, hook_path);
	}
	else 
*/	{
//		info("using helper (%s hook)", use_anticheat(gc) ? "compatibility" : "direct");
		success = create_inject_process(inject_path.c_str(), hook_dll);
	}

	return success;
}

bool NCCapture::create_inject_process( const wchar_t* inject_path, const wchar_t* hook_dll)
{
	LOG(DEBUG) << format_string("[NCCapture::create_inject_process]");
	std::wstring command_line;
	std::wstring inject_path_w;
	std::wstring hook_dll_w;

	wchar_t command_line_w[MAX_PATH];
	// TODO anticheat??
	bool anti_cheat = true;//use_anticheat(gc);
	PROCESS_INFORMATION pi = { 0 };
	STARTUPINFO si = { 0 };
	bool success = false;
	si.cb = sizeof(si);

	swprintf(command_line_w, MAX_PATH, L"\"%s\" \"%s\" %lu %lu", inject_path, hook_dll, (unsigned long)anti_cheat, anti_cheat ? m_pGC->thread_id : m_pGC->process_id);

	success = !!CreateProcessW(inject_path, 
								command_line_w, 
								NULL, NULL,
								false, 
								CREATE_NO_WINDOW, 
								NULL, NULL, 
								&si, &pi);
	if (success) 
	{
		CloseHandle(pi.hThread);
		m_pGC->injector_process = pi.hProcess;
		LOG(INFO) << format_string("[NCCapture::create_inject_process] create_inject_process() OK!!");
	}
	else 
	{
		LOG(ERROR) << format_string("[NCCapture::create_inject_process] Failed to create inject helper process: %lu", GetLastError());
	}

	return success;
}

bool NCCapture::init_texture_mutexes()
{
	m_pGC->texture_mutexes[0] = _open_mutex(MUTEX_TEXTURE1);
	m_pGC->texture_mutexes[1] = _open_mutex(MUTEX_TEXTURE2);

	if (!m_pGC->texture_mutexes[0] || !m_pGC->texture_mutexes[1]) 
	{
		DWORD error = GetLastError();
		if (error == 2) 
		{
			if (!m_pGC->retrying) 
			{
				m_pGC->retrying = 2;
				m_pGC->retry_interval = DEFAULT_RETRY_INTERVAL;
				LOG(INFO) << format_string("[NCCapture::init_texture_mutexes] hook not loaded yet, retrying..");
			}
		}
		else
		{
			LOG(ERROR) << format_string("[NCCapture::init_texture_mutexes] failed to open texture mutexes: %lu", GetLastError());
		}
		return false;
	}

	return true;
}

bool NCCapture::init_events()
{
	// event handle이 NCCapturer에서 생성되었는 지 체크
	if (!m_pGC->hook_restart) 
	{
		m_pGC->hook_restart = _open_event(EVENT_CAPTURE_RESTART);
		if (!m_pGC->hook_restart) 
		{
			LOG(ERROR) << format_string("[NCCapture::init_events] init_events: failed to get hook_restart event: %lu", GetLastError());
			return false;
		}
	}

	if (!m_pGC->hook_alive) 
	{
		m_pGC->hook_alive = _open_event(EVENT_HOOK_TARGETALIVE);
		if (!m_pGC->hook_alive) 
		{
			LOG(ERROR) << format_string("[NCCapture::init_events] failed to get hook_alive event: %lu", GetLastError());
			return false;
		}
	}

	if (!m_pGC->hook_init) 
	{
		m_pGC->hook_init = _open_event(EVENT_HOOK_INIT);
		if (!m_pGC->hook_init)
		{
			LOG(ERROR) << format_string("[NCCapture::init_events] failed to get hook_init event: %lu", GetLastError());
			return false;
		}
	}

	if (!m_pGC->hook_ready) 
	{
		m_pGC->hook_ready = _open_event(EVENT_HOOK_READY);
		if (!m_pGC->hook_ready)
		{
			LOG(ERROR) << format_string("[NCCapture::init_events] failed to get hook_ready event: %lu", GetLastError());
			return false;
		}
	}

	if (!m_pGC->hook_exit) 
	{
		m_pGC->hook_exit = _open_event(EVENT_HOOK_EXIT);
		if (!m_pGC->hook_exit)
		{
			LOG(ERROR) << format_string("[NCCapture::init_events] failed to get hook_exit event: %lu", GetLastError());
			return false;
		}
	}

	return true;
}

eCaptureResult NCCapture::init_capture_data()
{
	LOG(DEBUG) << format_string("[NCCapture::init_capture_data]");
	m_pGC->cx = m_pGC->global_hook_info->cx;
	m_pGC->cy = m_pGC->global_hook_info->cy;
	m_pGC->pitch = m_pGC->global_hook_info->pitch;

	if (m_pGC->data) 
	{
		UnmapViewOfFile(m_pGC->data);
		m_pGC->data = NULL;
	}

	CloseHandle(m_pGC->hook_data_map);

	m_pGC->hook_data_map = _open_map_plus_id(SHMEM_TEXTURE, m_pGC->global_hook_info->map_id);
	if (!m_pGC->hook_data_map)
	{
		DWORD error = GetLastError();
		if (error == 2) 
		{
			return eCR_RETRY;
		}
		else
		{
			LOG(ERROR) << format_string("[NCCapture::init_capture_data] failed to open file mapping: %lu", error);
		}
		return eCR_FAIL;
	}

	m_pGC->data = MapViewOfFile(m_pGC->hook_data_map, FILE_MAP_ALL_ACCESS, 0, 0, m_pGC->global_hook_info->map_size);
	if (!m_pGC->data) 
	{
		LOG(ERROR) << format_string("[NCCapture::init_capture_data] failed to map data view: %lu", GetLastError());
		return eCR_FAIL;
	}

	return eCR_SUCCESS;
}

std::wstring NCCapture::get_path()
{
	TCHAR buffer[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, buffer, MAX_PATH);
	std::wstring::size_type pos = std::wstring(buffer).find_last_of(L"\\/");
	return std::wstring(buffer).substr(0, pos);
}

bool NCCapture::init_shtex_capture()
{
	LOG(DEBUG) << format_string("[NCCapture::init_shtex_capture]");
	std::lock_guard<std::mutex> guard(g_mtx_lock_video);
	if (m_pGC->texture)
	{
		delete m_pGC->texture;
		m_pGC->texture = nullptr;
	}

	LOG(INFO) << format_string("[NCCapture::init_shtex_capture] shtex_data(0x%p) tex_handle(0x%p) type(%d) format(%d)", 
		m_pGC->shtex_data, m_pGC->shtex_data->tex_handle, m_pGC->global_hook_info->type, m_pGC->global_hook_info->format);

	while (true)
	{
		m_pGC->texture = VideoDevice::Instance()->OpenSharedResource((HANDLE)m_pGC->shtex_data->tex_handle);
		if (m_pGC->texture == nullptr)
		{
			if (!VideoDevice::Instance()->RecreateNextAdapter())
			{
				LOG(DEBUG) << format_string("[NCCapture::init_shtex_capture] failed to OpenSharedResource of all video devices");
				return false;
			}
			continue;	// try next device
		}
		break;	// success OpenSharedResource
	}

	if (m_pGC->texture && m_pGC->global_hook_info)
	{
		m_pGC->texture->SetFlip(m_pGC->global_hook_info->flip);
	}

	return true;
}

bool NCCapture::init_shmem_capture()
{
	LOG(DEBUG) << format_string("[NCCapture::init_shmem_capture]");
	eColorFormat format;

	m_pGC->texture_buffers[0] = (uint8_t*)m_pGC->data + m_pGC->shmem_data->tex1_offset;
	m_pGC->texture_buffers[1] = (uint8_t*)m_pGC->data + m_pGC->shmem_data->tex2_offset;

	m_pGC->convert_16bit = is_16bit_format(m_pGC->global_hook_info->format);
	format = m_pGC->convert_16bit ? eCF_BGRA : convert_format(m_pGC->global_hook_info->format);
	/*
		if (gc->image)
		{
			delete gc->image;
			gc->image = nullptr;
		}
	*/
	int buffer_size = m_pGC->pitch * m_pGC->cy;
	//	gc->image = new ScratchImage();
	//	((ScratchImage*)gc->image)->Initialize2D(DXGI_FORMAT_B8G8R8A8_UNORM, gc->cx, gc->cy, 1, 1);
	/*
		obs_enter_graphics();
		gs_texture_destroy(gc->texture);
		gc->texture = gs_texture_create(gc->cx, gc->cy, format, 1, NULL, GS_DYNAMIC);
		obs_leave_graphics();

		if (!gc->texture)
		{
			LOG(DEBUG) << format_string("[NCCapture::init_shmem_capture] failed to create texture");
			return false;
		}
	*/
	m_pGC->copy_texture = copy_shmem_tex;

	return true;
}
HANDLE NCCapture::_open_event(const wchar_t* name)
{
	wchar_t new_name[64];
	_snwprintf(new_name, 64, L"%s%lu", name, m_pGC->process_id);
	if (m_pGC->is_app)
	{
		// TODO
		//return open_app_event(gc->app_sid, new_name);
		return 0;
	}
	else
		return OpenEventW(GC_EVENT_FLAGS, false, new_name);
}

void NCCapture::_reset_event(HANDLE event)
{
	if (!event)
		return;

	ResetEvent((HANDLE)event);
}

HANDLE NCCapture::_open_hook_info()
{
	wchar_t new_name[64];
	_snwprintf(new_name, 64, L"%s%lu", SHMEM_HOOK_INFO, m_pGC->process_id);

	LOG(DEBUG) << format_string("[NCCapture::_open_hook_info] map id: %S", new_name);

	if (m_pGC->is_app)
	{
		//		return open_app_map(gc->app_sid, new_name);
		return 0;
	}
	else
	{
		return OpenFileMappingW(GC_MAPPING_FLAGS, false, new_name);
	}
}

HANDLE NCCapture::_open_mutex(const wchar_t *name)
{
	wchar_t new_name[64];
	_snwprintf(new_name, 64, L"%s%lu", name, m_pGC->process_id);

	if (m_pGC->is_app)
	{
		// TODO
		//return open_app_mutex(gc->app_sid, new_name)
		return 0;
	}
	else
		return OpenMutexW(GC_MUTEX_FLAGS, false, new_name);
}

HANDLE NCCapture::_open_map_plus_id(const wchar_t *name, DWORD id)
{
	wchar_t new_name[64];
	_snwprintf(new_name, 64, L"%s%lu", name, id);

	//	debug("map id: %S", new_name);
	if (m_pGC->is_app)
	{
		//open_app_map(gc->app_sid, new_name);
		return 0;
	}
	else
	{
		return OpenFileMappingW(GC_MAPPING_FLAGS, false, new_name);
	}
}
