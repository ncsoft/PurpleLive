#pragma once
#include "common.h"
#include "Singleton.h"
#include <Windows.h>
#include <stdint.h>
#include <string>

typedef HANDLE(WINAPI *open_process_proc_t)(DWORD, BOOL, DWORD);

#define INITIAL_RETRY_INTERVAL	0.5f
#define DEFAULT_RETRY_INTERVAL	2.0f

enum eCaptureResult
{
	eCR_FAIL, 
	eCR_RETRY, 
	eCR_SUCCESS 
};

class NCAudioCapture;
struct GameCapture;
class NCCAPTURESDK_API NCCapture : public NCCaptureSDK::Singleton<NCCapture>
{
public:
	NCCapture();
	~NCCapture();

	void	InitLog(const char* path);
	void	SetEnableLog(bool error, bool info, bool debug);

	void	RegisterDeviceCreatedCallback(DEVICE_CREATED_CALLBACK_FUNC callback, void* user_data);

	bool	StartCapture();
	bool	StartCapture(const char* title, bool record = false);
	bool	StartCapture(HWND hwnd, bool record=false);
	bool	IsStartedCapture();
	bool	IsReadyCapture();
	bool	IsCapturing();
	
	void	StopCapture();

	bool	CreateVideoDevice(HWND hwnd, unsigned int width, unsigned int height);
	void	ResizeVideoDevice(int width, int height);

	void	Update();

	void	DrawRect(const FRECT& dest, uint32_t color=0xFFFFFFFF);
	void	DrawRectInGame(const SIZE& src, const FRECT& dest, uint32_t color = 0xFFFFFFFF);

	void	BeginRender();
	void	EndRender();

	void	OnActivated();
	void	OnDeactivated();
	void	OnSuspending();
	void	OnResuming();
	void	OnResizing(int width, int height);

	const CapturedImage*	GetFrameBufferImage();
//	CapturedImage*			GetCapturedImage();
	void*					GetCapturedTexture();	//return ID3D11Texture2D*(DXGI_FORMAT_R8G8B8A8_UNORM)
	void*					GetCapturedSRV(int& width, int& height, bool& flip);	// deprecated v0.0.1.4
	void*					GetCapturedSRV(int& width, int& height, bool& flip, int& hook_ready_key);

	bool	_is_active_update_thread() { return m_b_update_thread; }
protected:
	static DWORD WINAPI	ThreadProc(LPVOID p);

	void	try_start_capture();
	void	try_hook();
	void	update();
	bool	init_hook();
	bool	start_capture();
	void    stop_capture();
	void	ready_capture();
	void	init_capture_config();
	void	request_target_alive();
	void	setup_window(HWND hwnd);
	HWND	get_selected_window();
	bool	target_suspended();
	bool	open_target_process();
	bool	is_64bit_process(HANDLE process);
	bool	is_64bit_windows();
	bool	init_keepalive();
	bool	attempt_existing_hook();
	bool	inject_hook();
	bool	init_hook_info();
	bool	create_inject_process(const wchar_t* inject_path, const wchar_t* hook_dll);
	bool	init_texture_mutexes();
	bool	init_events();
	void	get_offsets();
	eCaptureResult	init_capture_data();
	std::wstring get_path();

	// utility function
	bool	init_shtex_capture();
	bool	init_shmem_capture();
	HANDLE	_open_event(const wchar_t* name);
	void	_reset_event(HANDLE event);
	HANDLE	_open_hook_info();
	HANDLE	_open_mutex(const wchar_t *name);
	HANDLE	_open_map_plus_id(const wchar_t *name, DWORD id);

private:
	HWND				m_hwnd = nullptr;
	bool				m_record = false;

	HANDLE				m_h_update_thread=0;
	bool				m_b_update_thread = false;
	GameCapture*		m_pGC = nullptr;
	open_process_proc_t	m_open_process_proc = 0;
	HMODULE				m_kernel32_handle = 0;
	CapturedImage		m_captured_image;
};

