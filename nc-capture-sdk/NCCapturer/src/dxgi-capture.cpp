#include <d3d10_1.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <d3dcompiler.h>

#include "d3d1x_shaders.hpp"
#include "graphics-hook.h"
#include "NCCaptureConfig.h"
#include "detourhook.h"

#if COMPILE_D3D12_HOOK
#include <d3d12.h>
#endif

typedef HRESULT(STDMETHODCALLTYPE *resize_buffers_t)(IDXGISwapChain *, UINT,
						     UINT, UINT, DXGI_FORMAT,
						     UINT);
typedef HRESULT(STDMETHODCALLTYPE *present_t)(IDXGISwapChain *, UINT, UINT);
typedef HRESULT(STDMETHODCALLTYPE *present1_t)(IDXGISwapChain1 *, UINT, UINT,
					       const DXGI_PRESENT_PARAMETERS *);

static struct detour_hook resize_buffers;
static struct detour_hook present;
static struct detour_hook present1;

struct dxgi_swap_data {
	IDXGISwapChain *swap;
	void (*capture)(void *, void *, bool);
	void (*free)(void);
};

static struct dxgi_swap_data data = {};

static bool setup_dxgi(IDXGISwapChain *swap)
{
	hlog("[NCCAPTURER::dxgi-capture.cpp!::setup_dxgi] swap(0x%x)", swap);

	IUnknown *device;
	HRESULT hr;

	hr = swap->GetDevice(__uuidof(ID3D11Device), (void **)&device);
	if (SUCCEEDED(hr)) {
		ID3D11Device *d3d11 = reinterpret_cast<ID3D11Device *>(device);
		D3D_FEATURE_LEVEL level = d3d11->GetFeatureLevel();
		device->Release();

		if (level >= D3D_FEATURE_LEVEL_11_0) {
			data.swap = swap;
			data.capture = d3d11_capture;
			data.free = d3d11_free;
			hlog("[NCCAPTURER::dxgi-capture.cpp!::setup_dxgi] success ID3D11Device");
			return true;
		}
	}

	hr = swap->GetDevice(__uuidof(ID3D10Device), (void **)&device);
	if (SUCCEEDED(hr)) {
		data.swap = swap;
		data.capture = d3d10_capture;
		data.free = d3d10_free;
		device->Release();
		hlog("[NCCAPTURER::dxgi-capture.cpp!::setup_dxgi] success ID3D10Device");
		return true;
	}

	hr = swap->GetDevice(__uuidof(ID3D11Device), (void **)&device);
	if (SUCCEEDED(hr)) {
		data.swap = swap;
		data.capture = d3d11_capture;
		data.free = d3d11_free;
		device->Release();
		hlog("[NCCAPTURER::dxgi-capture.cpp!::setup_dxgi] success ID3D11Device");
		return true;
	}

#if COMPILE_D3D12_HOOK
	hr = swap->GetDevice(__uuidof(ID3D12Device), (void **)&device);
	if (SUCCEEDED(hr)) {
		data.swap = swap;
		data.capture = d3d12_capture;
		data.free = d3d12_free;
		device->Release();
		hlog("[NCCAPTURER::dxgi-capture.cpp!::setup_dxgi] success ID3D12Device");
		return true;
	}
#endif

	hlog("[NCCAPTURER::dxgi-capture.cpp!::setup_dxgi] failed");

	return false;
}

static bool resize_buffers_called = false;

static HRESULT STDMETHODCALLTYPE hook_resize_buffers(IDXGISwapChain *swap,
						     UINT buffer_count,
						     UINT width, UINT height,
						     DXGI_FORMAT format,
						     UINT flags)
{
	HRESULT hr;

	if (!!data.free)
		data.free();

	data.swap = nullptr;
	data.free = nullptr;
	data.capture = nullptr;

	resize_buffers_t call = (resize_buffers_t)resize_buffers.pOrigin;
	hr = call(swap, buffer_count, width, height, format, flags);

	resize_buffers_called = true;

	return hr;
}

static inline IUnknown *get_dxgi_backbuffer(IDXGISwapChain *swap)
{
	IDXGIResource *res = nullptr;
	HRESULT hr;

	hr = swap->GetBuffer(0, __uuidof(IUnknown), (void **)&res);
	if (FAILED(hr))
		hlog_hr("get_dxgi_backbuffer: GetBuffer failed", hr);

	return res;
}

static HRESULT STDMETHODCALLTYPE hook_present(IDXGISwapChain *swap,
					      UINT sync_interval, UINT flags)
{
	IUnknown *backbuffer = nullptr;
	bool capture_overlay = global_hook_info->capture_overlay;
	bool test_draw = (flags & DXGI_PRESENT_TEST) != 0;
	bool capture;
	HRESULT hr;

	if (!data.swap && !capture_active()) {
		hlog("[NCCAPTURER::dxgi-capture.cpp!::hook_present] setup_dxgi(swap)");
		setup_dxgi(swap);
	}

	capture = !test_draw && swap == data.swap && !!data.capture;
	if (capture && !capture_overlay) {
		backbuffer = get_dxgi_backbuffer(swap);

		if (!!backbuffer) {
			data.capture(swap, backbuffer, capture_overlay);
			backbuffer->Release();
		}
	}

	present_t call = (present_t)present.pOrigin;
	hr = call(swap, sync_interval, flags);

	if (capture && capture_overlay) {
		/*
		 * It seems that the first call to Present after ResizeBuffers
		 * will cause the backbuffer to be invalidated, so do not
		 * perform the post-overlay capture if ResizeBuffers has
		 * recently been called.  (The backbuffer returned by
		 * get_dxgi_backbuffer *will* be invalid otherwise)
		 */
		if (resize_buffers_called) {
			resize_buffers_called = false;
		} else {
			backbuffer = get_dxgi_backbuffer(swap);

			if (!!backbuffer) {
				data.capture(swap, backbuffer, capture_overlay);
				backbuffer->Release();
			}
		}
	}

	return hr;
}

static HRESULT STDMETHODCALLTYPE
hook_present1(IDXGISwapChain1 *swap, UINT sync_interval, UINT flags,
	      const DXGI_PRESENT_PARAMETERS *params)
{
	IUnknown *backbuffer = nullptr;
	bool capture_overlay = global_hook_info->capture_overlay;
	bool test_draw = (flags & DXGI_PRESENT_TEST) != 0;
	bool capture;
	HRESULT hr;

	if (!data.swap && !capture_active()) {
		hlog("[NCCAPTURER::dxgi-capture.cpp!::hook_present1] setup_dxgi(swap)");
		setup_dxgi(swap);
	}

	capture = !test_draw && swap == data.swap && !!data.capture;
	if (capture && !capture_overlay) {
		backbuffer = get_dxgi_backbuffer(swap);

		if (!!backbuffer) {
			DXGI_SWAP_CHAIN_DESC1 desc;
			swap->GetDesc1(&desc);
			data.capture(swap, backbuffer, capture_overlay);
			backbuffer->Release();
		}
	}

	present1_t call = (present1_t)present1.pOrigin;
	hr = call(swap, sync_interval, flags, params);

	if (capture && capture_overlay) {
		if (resize_buffers_called) {
			resize_buffers_called = false;
		} else {
			backbuffer = get_dxgi_backbuffer(swap);

			if (!!backbuffer) {
				data.capture(swap, backbuffer, capture_overlay);
				backbuffer->Release();
			}
		}
	}

	return hr;
}

static pD3DCompile get_compiler(void)
{
	pD3DCompile compile = nullptr;
	char d3dcompiler[40] = {};
	int ver = 49;

	while (ver > 30) {
		sprintf_s(d3dcompiler, 40, "D3DCompiler_%02d.dll", ver);

		HMODULE module = LoadLibraryA(d3dcompiler);
		if (module) {
			compile = (pD3DCompile)GetProcAddress(module,
							      "D3DCompile");
			if (compile) {
				break;
			}
		}

		ver--;
	}

	return compile;
}

static uint8_t vertex_shader_data[1024];
static uint8_t pixel_shader_data[1024];
static size_t vertex_shader_size = 0;
static size_t pixel_shader_size = 0;

bool hook_dxgi(void)
{
	pD3DCompile compile;
	ID3D10Blob *blob;
	HMODULE dxgi_module = get_system_module("dxgi.dll");
	HRESULT hr;
	void *present_addr;
	void *resize_addr;
	void *present1_addr = nullptr;

	if (!dxgi_module) {
		return false;
	}

	compile = get_compiler();
	if (!compile) {
		hlog("hook_dxgi: failed to find d3d compiler library");
		return true;
	}

	/* ---------------------- */

	hr = compile(vertex_shader_string, sizeof(vertex_shader_string),
		     "vertex_shader_string", nullptr, nullptr, "main", "vs_4_0",
		     D3D10_SHADER_OPTIMIZATION_LEVEL1, 0, &blob, nullptr);
	if (FAILED(hr)) {
		hlog_hr("hook_dxgi: failed to compile vertex shader", hr);
		return true;
	}

	vertex_shader_size = (size_t)blob->GetBufferSize();
	memcpy(vertex_shader_data, blob->GetBufferPointer(),
	       blob->GetBufferSize());
	blob->Release();

	/* ---------------------- */

	hr = compile(pixel_shader_string, sizeof(pixel_shader_string),
		     "pixel_shader_string", nullptr, nullptr, "main", "ps_4_0",
		     D3D10_SHADER_OPTIMIZATION_LEVEL1, 0, &blob, nullptr);
	if (FAILED(hr)) {
		hlog_hr("hook_dxgi: failed to compile pixel shader", hr);
		return true;
	}

	pixel_shader_size = (size_t)blob->GetBufferSize();
	memcpy(pixel_shader_data, blob->GetBufferPointer(),
	       blob->GetBufferSize());
	blob->Release();

	/* ---------------------- */

	present_addr = get_offset_addr(dxgi_module,
				       global_hook_info->offsets.dxgi.present);
	resize_addr = get_offset_addr(dxgi_module,
				      global_hook_info->offsets.dxgi.resize);
	if (global_hook_info->offsets.dxgi.present1)
		present1_addr = get_offset_addr(
			dxgi_module, global_hook_info->offsets.dxgi.present1);

	hlog("[NCCAPTURER::dxgi-capture.cpp!::hook_dxgi] hook_init (present) dxgi_module(0x%p) offset(0x%x) origin(0x%p) hook(0x%x)", dxgi_module, global_hook_info->offsets.dxgi.present, present_addr, hook_present);

	detour_hook_start(&present, present_addr, (void *)hook_present, "IDXGISwapChain::Present");
	detour_hook_start(&resize_buffers, resize_addr, (void *)hook_resize_buffers, "IDXGISwapChain::ResizeBuffers");
	if (present1_addr)
		detour_hook_start(&present1, present1_addr, (void *)hook_present1, "IDXGISwapChain1::Present1");

	hlog("Hooked DXGI");
	return true;
}

void unhook_dxgi(void)
{
	hlog("[NCCAPTURER::dxgi-capture.cpp!::unhook_dxgi]");

	detour_hook_stop(&present);
	detour_hook_stop(&resize_buffers);
	detour_hook_stop(&present1);
}

bool hook_dxgi_reset_when_jmp_patched(void)
{
	hlog("[NCCAPTURER::dxgi-capture.cpp!::hook_dxgi_reset_when_jmp_patched]");

	bool hook_complete_1 = detour_hook_is_complete_when_jmp_patched(&present);
	bool hook_complete_2 = detour_hook_is_complete_when_jmp_patched(&resize_buffers);
	bool hook_complete_3 = detour_hook_is_complete_when_jmp_patched(&present1);
	bool res = false;

	int i=0;
	for (; i<DETOUR_JUMP_PATCHED_MAX_RETRY_COUNT; i++)
	{
		if (!hook_complete_1)
			hook_complete_1 = detour_hook_restart_when_jmp_patched(&present);
		if (!hook_complete_2)
			hook_complete_2 = detour_hook_restart_when_jmp_patched(&resize_buffers);
		if (!hook_complete_3)
			hook_complete_3 = detour_hook_restart_when_jmp_patched(&present1);

		if (hook_complete_1 && hook_complete_2 && hook_complete_3) {
			res = true;
			break;
		}
	}
	hlog("[NCCAPTURER::dxgi-capture.cpp!::hook_dxgi_reset_when_jmp_patched] loop(%d) res(%d)", i, res);
	return true;
}

uint8_t *get_d3d1x_vertex_shader(size_t *size)
{
	*size = vertex_shader_size;
	return vertex_shader_data;
}

uint8_t *get_d3d1x_pixel_shader(size_t *size)
{
	*size = pixel_shader_size;
	return pixel_shader_data;
}
