#include "VideoDevice.h"
#include "easylogging++.h"
#include <atlbase.h>
#include "Texture2D.h"
#include "DXUtil.h"
#include <DirectXColors.h>
#include <algorithm>
#include <SimpleMath.h>

using namespace std;
using namespace DirectX;
using Microsoft::WRL::ComPtr;

const static D3D_FEATURE_LEVEL featureLevels[] = {
	D3D_FEATURE_LEVEL_11_0,
	D3D_FEATURE_LEVEL_10_1,
	D3D_FEATURE_LEVEL_10_0,
	D3D_FEATURE_LEVEL_9_3,
};

static RECT GetRectToFitScreen(int sx, int sy, int dx, int dy)
{
	double ratio_src = (double)sx / sy;
	double ratio_dst = (double)dx / dy;

	int x, y, w, h;
	if (ratio_src > ratio_dst)
	{
		// letter-box : top, bottom
		w = dx;
		h = round((double)w / ratio_src);
		x = 0;
		y = (dy - h) / 2;
	}
	else
	{
		// letter-box : left, right
		h = dy;
		w = round(ratio_src * h);
		x = (dx - w) / 2;
		y = 0;
	}

	RECT rc{ x, y, w + x, h + y };
	//LOG(INFO) << format_string("[AppUtils::GetAspectRatioRect] (%d %d) (%d %d)", x, y, w, h);
	return rc;
}

VideoDevice::VideoDevice()
{
	m_featureLevel = D3D_FEATURE_LEVEL_9_1;
}

VideoDevice::~VideoDevice()
{
	Destroy();
	LOG(INFO) << "[VideoDevice::~VideoDevice] Singleton";
}

void VideoDevice::RegisterCreatedCallback(DEVICE_CREATED_CALLBACK_FUNC callback, void* user_data)
{
	m_created_callback = callback;
	m_callback_user_data = user_data;
}

bool VideoDevice::Create(HWND hwnd, unsigned int width, unsigned int height)
{
	m_window = hwnd;
	m_width = width;
	m_height = height;

	if (_Create(0) == false)
	{
		return false;
	}

	if (m_created_callback)
		m_created_callback(m_callback_user_data, false);

	m_bInit = true;

	LOG(VERBOSE) << format_string("[VideoDevice::Create] hwnd(0x%p) size(%d %d)", hwnd, width, height);

	return true;
}

void VideoDevice::Resize(int width, int height)
{
	m_width = std::max(width, 1);
	m_height = std::max(height, 1);

	CreateResources();
}

void VideoDevice::Destroy()
{
	m_spriteBatch.reset();
	m_depthStencilView.Reset();
	m_renderTargetView.Reset();
	m_swapChain.Reset();
	m_d3dContext.Reset();
	m_d3dDevice.Reset();
	m_states.reset();
	m_primitiveBatch.reset();
	m_batchEffect.reset();
	m_batchInputLayout.Reset();
}

void VideoDevice::Update()
{
	if (m_bInit == false)
		return;

	m_timer.Tick([&]()
	{
		_Update();
	});
}

void VideoDevice::BeginRender()
{
	if (m_timer.GetFrameCount() == 0)
		return;

	_Clear();
}

void VideoDevice::EndRender()
{
	if (m_timer.GetFrameCount() == 0)
		return;

	_Present();
}

bool VideoDevice::_Recreate(int adapter_index)
{
	Destroy();

	bool result = _Create(adapter_index);

	if (result && m_created_callback)
		m_created_callback(m_callback_user_data, true);

	return result;
}

bool VideoDevice::RecreateNextAdapter()
{
	return _Recreate(m_current_adapter_index+1);
}

Texture2D* VideoDevice::OpenSharedResource(HANDLE handle)
{
	Texture2D* txtr = new Texture2D();
	if (txtr->CreateAsSharedResource(m_d3dDevice.Get(), handle) == false)
	{
		delete txtr;
		return nullptr;
	}

	return txtr;
}

const CapturedImage* VideoDevice::GetFrameBufferImage()
{
	ComPtr<ID3D11Texture2D> backBuffer;
	HRESULT hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID*>(backBuffer.GetAddressOf()));
	if (SUCCEEDED(hr))
	{
//		ComPtr<ID3D11Texture2D> pStaging;
		hr = CaptureTexture(m_d3dDevice.Get(), m_d3dContext.Get(), backBuffer.Get(), m_scratch_image);
		DXUtil::ThrowIfFailed(hr);
		
		const DirectX::Image* pImage = m_scratch_image.GetImages();
		m_image.width = pImage->width;
		m_image.height = pImage->height;
		m_image.rowPitch = pImage->rowPitch;
		m_image.slicePitch = pImage->slicePitch;
		m_image.pixels = pImage->pixels;

		return &m_image;
	}
	else
	{
		return nullptr;
	}
}

void VideoDevice::DrawRect(const FRECT& dest, uint32_t color)
{
	m_d3dContext->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	m_d3dContext->OMSetDepthStencilState(m_states->DepthNone(), 0);
	m_d3dContext->RSSetState(m_states->CullCounterClockwise());

	m_batchEffect->Apply(m_d3dContext.Get());
	m_d3dContext->IASetInputLayout(m_batchInputLayout.Get());

	m_primitiveBatch->Begin();

	DirectX::PackedVector::XMCOLOR uiColor(color);
	XMVECTOR fColor = DirectX::PackedVector::XMLoadColor(&uiColor);

	VertexPositionColor v0(DirectX::SimpleMath::Vector3(dest.left, dest.top, 0.5f), fColor);
	VertexPositionColor v1(DirectX::SimpleMath::Vector3(dest.right, dest.top, 0.5f), fColor);
	VertexPositionColor v2(DirectX::SimpleMath::Vector3(dest.right, dest.bottom, 0.5f), fColor);
	VertexPositionColor v3(DirectX::SimpleMath::Vector3(dest.left, dest.bottom, 0.5f), fColor);

	m_primitiveBatch->DrawQuad(v0, v1, v2, v3);

	m_primitiveBatch->End();
}

void VideoDevice::DrawRectInGame(const SIZE& src, const FRECT& dest, uint32_t color)
{
	// Size는 비율만 참고함
	RECT temp = GetRectToFitScreen((float)src.cx, (float)src.cy, (float)m_width, (float)m_height);

	float new_width = (float)(temp.right - temp.left);
	float new_height = (float)(temp.bottom - temp.top);

	float x_gap = (m_width - new_width) / 2;
	float y_gap = (m_height - new_height) / 2;

	FRECT _rcRevise = { (dest.left*new_width) + x_gap,
						(dest.top*new_height) + y_gap,
						(dest.right*new_width) + x_gap,
						(dest.bottom*new_height) + y_gap };

	FRECT result;
	float half_width = m_width / 2.0f;
	float half_height = m_height / 2.0f;
	result.left = (_rcRevise.left - half_width) / half_width;
	result.top = -(_rcRevise.top - half_height) / half_height;
	result.right = (_rcRevise.right - half_width) / half_width;
	result.bottom = -(_rcRevise.bottom - half_height) / half_height;

	m_d3dContext->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
	m_d3dContext->OMSetDepthStencilState(m_states->DepthNone(), 0);
	m_d3dContext->RSSetState(m_states->CullCounterClockwise());

	m_batchEffect->Apply(m_d3dContext.Get());
	m_d3dContext->IASetInputLayout(m_batchInputLayout.Get());

	m_primitiveBatch->Begin();

	DirectX::PackedVector::XMCOLOR uiColor(color);
	XMVECTOR fColor = DirectX::PackedVector::XMLoadColor(&uiColor);

	VertexPositionColor v0(DirectX::SimpleMath::Vector3(result.left, result.top, 0.5f), fColor);
	VertexPositionColor v1(DirectX::SimpleMath::Vector3(result.right, result.top, 0.5f), fColor);
	VertexPositionColor v2(DirectX::SimpleMath::Vector3(result.right, result.bottom, 0.5f), fColor);
	VertexPositionColor v3(DirectX::SimpleMath::Vector3(result.left, result.bottom, 0.5f), fColor);

	m_primitiveBatch->DrawQuad(v0, v1, v2, v3);

	m_primitiveBatch->End();
}

/*
unsigned char * VideoDevice::GetImageBits()
{
	ComPtr<ID3D11Texture2D> backBuffer;
	HRESULT hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID*>(backBuffer.GetAddressOf()));
	if (SUCCEEDED(hr))
	{
		ComPtr<ID3D11Texture2D> pStaging;
		ScratchImage image;
		hr = CaptureTexture(m_d3dDevice.Get(), m_d3dContext.Get(), backBuffer.Get(), image);
//		hr = SaveDDSTextureToFile(immContext.Get(), backBuffer.Get(), L"SCREENSHOT.DDS");
		DXUtil::ThrowIfFailed(hr);
	}
	else
	{
		return nullptr;
	}
}
*/
void VideoDevice::OnActivated()
{
	//
}

void VideoDevice::OnDeactivated()
{
	//
}

void VideoDevice::OnSuspending()
{
	//
}

void VideoDevice::OnResuming()
{
	//
}

void VideoDevice::OnResizing(int width, int height)
{
	if (m_bInit == false)
		return;

//	Resize(width, height);
}

static void EnumAdapters(std::vector<IDXGIAdapter1*>& adapters)
{
	ComPtr<IDXGIFactory1> factory;
	IDXGIAdapter1* adapter = nullptr;
	HRESULT hr;
	UINT i;

	LOG(INFO) << "Available Video Adapters: ";

//	IID factoryIID = (GetWinVer() >= 0x602) ? __uuidof(IDXGIFactory2) : __uuidof(IDXGIFactory1);
	IID factoryIID = __uuidof(IDXGIFactory1);

	hr = CreateDXGIFactory1(factoryIID, (void **)factory.GetAddressOf());
	DXUtil::ThrowIfFailed(hr);

	for (i = 0; factory->EnumAdapters1(i, &adapter) == S_OK; ++i)
	{
		DXGI_ADAPTER_DESC desc;
		char name[512] = "";

		hr = adapter->GetDesc(&desc);
		if (FAILED(hr))
			continue;

		/* ignore Microsoft's 'basic' renderer' */
		if (desc.VendorId == 0x1414 && desc.DeviceId == 0x8c)
		{
			DXUtil::SafeRelease(adapter);
			continue;
		}

		adapters.push_back(adapter);

		std::wstring descNameW = desc.Description;
		std::string descName;
		descName.assign(descNameW.begin(), descNameW.end());
	
		LOG(INFO) << "\tAdapter " << i << " " << descName.c_str();
		LOG(INFO) << "\t  Dedicated VRAM: "<< desc.DedicatedVideoMemory;
		LOG(INFO) << "\t  Shared VRAM: " << desc.SharedSystemMemory;
	}
}

void VideoDevice::DrawSpriteFull(ID3D11ShaderResourceView * srv, const SIZE& srcSize, bool flip, DirectX::FXMVECTOR color)
{
	SpriteEffects effects = flip ? SpriteEffects_FlipVertically : SpriteEffects_None;

	m_spriteBatch->Begin(SpriteSortMode_BackToFront, m_blendState.Get());

	RECT dest = GetRectToFitScreen((float)srcSize.cx, (float)srcSize.cy, (float)m_width, (float)m_height);

	m_spriteBatch->Draw(srv, dest, nullptr, color, 0.0f, XMFLOAT2(0, 0), effects, 0);

	m_spriteBatch->End();
}

void VideoDevice::DrawSpriteInGame(ID3D11ShaderResourceView * srv, const SIZE & gameSize, const FRECT& dest, DirectX::FXMVECTOR color)
{
	// gameSize는 비율만 참고함
	RECT temp = GetRectToFitScreen((float)gameSize.cx, (float)gameSize.cy, (float)m_width, (float)m_height);

	float new_width = (float)(temp.right - temp.left);
	float new_height = (float)(temp.bottom - temp.top);

	float x_gap = (m_width - new_width) / 2;
	float y_gap = (m_height - new_height) / 2;

	RECT _rcRevise = {	(long)((dest.left*new_width) + x_gap + 0.5f),
						(long)((dest.top*new_height) + y_gap + 0.5f),
						(long)((dest.right*new_width) + x_gap + 0.5f),
						(long)((dest.bottom*new_height) + y_gap + 0.5f) };

	m_spriteBatch->Begin(SpriteSortMode_BackToFront, m_blendState.Get());

	m_spriteBatch->Draw(srv, _rcRevise, nullptr, color, 0.0f, XMFLOAT2(0, 0), SpriteEffects_None, 0);

	m_spriteBatch->End();
}

void VideoDevice::DrawSprite(ID3D11ShaderResourceView* srv, const DirectX::XMFLOAT2& pos, DirectX::FXMVECTOR color)
{
	m_spriteBatch->Begin(SpriteSortMode_BackToFront, m_blendState.Get());

	m_spriteBatch->Draw(srv, pos, color);

	m_spriteBatch->End();
}

void VideoDevice::DrawSprite(ID3D11ShaderResourceView * srv, const RECT & dest, bool flip, DirectX::FXMVECTOR color)
{
	SpriteEffects effects = flip ? SpriteEffects_FlipVertically : SpriteEffects_None;

	m_spriteBatch->Begin(SpriteSortMode_BackToFront, m_blendState.Get());

	m_spriteBatch->Draw(srv, dest, nullptr, color, 0.0f, XMFLOAT2(0,0), effects, 0);

	m_spriteBatch->End();
}

bool VideoDevice::_Create(int adapter_index)
{
	if (CreateDevice(adapter_index))
	{
		CreateResources();
		CreateBlendState();
		LOG(INFO) << format_string("[VideoDevice::_Create] CreateDevice success adapter_index(%d)", adapter_index);
		return true;
	}

	LOG(ERROR) << format_string("[VideoDevice::_Create] CreateDevice failed adapter_index(%d)", adapter_index);
	return false;
}

bool VideoDevice::CreateDevice(int adapter_index)
{
	UINT creationFlags = 0;

#ifdef _DEBUG
	//    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	static const D3D_FEATURE_LEVEL featureLevels[] =
	{
		// TODO: Modify for supported Direct3D feature levels
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1,
	};

	std::vector<IDXGIAdapter1*> adapters;
	EnumAdapters(adapters);

	if (adapters.size() == 0)
	{
		LOG(ERROR) << "Can not find HW Video Adapter!!";
		return false;
	}
	else if (adapters.size() <= adapter_index)
	{
		LOG(ERROR) << "Video Adapters array index exceeded";
		return false;
	}

	m_current_adapter_index = adapter_index;

	// Create the DX11 API device object, and get a corresponding context.
	ComPtr<ID3D11Device> device;
	ComPtr<ID3D11DeviceContext> context;
	DXUtil::ThrowIfFailed(D3D11CreateDevice(
		adapters[adapter_index],
		D3D_DRIVER_TYPE_UNKNOWN,
		nullptr,
		creationFlags,
		featureLevels,
		_countof(featureLevels),
		D3D11_SDK_VERSION,
		device.ReleaseAndGetAddressOf(),    // returns the Direct3D device created
		&m_featureLevel,                    // returns feature level of device created
		context.ReleaseAndGetAddressOf()    // returns the device immediate context
	));

	std::for_each(adapters.begin(), adapters.end(), [](auto& adapter) { DXUtil::SafeRelease(adapter); });

#ifndef NDEBUG
	ComPtr<ID3D11Debug> d3dDebug;
	if (SUCCEEDED(device.As(&d3dDebug)))
	{
		ComPtr<ID3D11InfoQueue> d3dInfoQueue;
		if (SUCCEEDED(d3dDebug.As(&d3dInfoQueue)))
		{
#ifdef _DEBUG
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif
			D3D11_MESSAGE_ID hide[] =
			{
				D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
				// TODO: Add more message IDs here as needed.
			};
			D3D11_INFO_QUEUE_FILTER filter = {};
			filter.DenyList.NumIDs = _countof(hide);
			filter.DenyList.pIDList = hide;
			d3dInfoQueue->AddStorageFilterEntries(&filter);
		}
	}
#endif

	DXUtil::ThrowIfFailed(device.As(&m_d3dDevice));
	DXUtil::ThrowIfFailed(context.As(&m_d3dContext));

	// TODO: Initialize device dependent objects here (independent of window size).
	m_spriteBatch = std::make_unique<SpriteBatch>(m_d3dContext.Get());
	m_states = std::make_unique<CommonStates>(m_d3dDevice.Get());
	m_primitiveBatch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(m_d3dContext.Get());
	m_batchEffect = std::make_unique<BasicEffect>(m_d3dDevice.Get());
	m_batchEffect->SetVertexColorEnabled(true);
	{
		void const* shaderByteCode;
		size_t byteCodeLength;

		m_batchEffect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

		DXUtil::ThrowIfFailed(
			device->CreateInputLayout(VertexPositionColor::InputElements,
				VertexPositionColor::InputElementCount,
				shaderByteCode, byteCodeLength,
				m_batchInputLayout.ReleaseAndGetAddressOf())
		);
	}
	return true;
}

void VideoDevice::CreateResources()
{
	ID3D11RenderTargetView* nullViews[] = { nullptr };
	m_d3dContext->OMSetRenderTargets(_countof(nullViews), nullViews, nullptr);
	m_renderTargetView.Reset();
	m_depthStencilView.Reset();
	m_d3dContext->Flush();

	const UINT backBufferWidth = static_cast<UINT>(m_width);
	const UINT backBufferHeight = static_cast<UINT>(m_height);
	const DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
	const DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	constexpr UINT backBufferCount = 2;

	// If the swap chain already exists, resize it, otherwise create one.
	if (m_swapChain)
	{
		HRESULT hr = m_swapChain->ResizeBuffers(backBufferCount, backBufferWidth, backBufferHeight, backBufferFormat, 0);

		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			// If the device was removed for any reason, a new device and swap chain will need to be created.
			OnDeviceLost();

			// Everything is set up now. Do not continue execution of this method. OnDeviceLost will reenter this method 
			// and correctly set up the new device.
			return;
		}
		else
		{
			DXUtil::ThrowIfFailed(hr);
		}
	}
	else
	{
		// First, retrieve the underlying DXGI Device from the D3D Device.
		ComPtr<IDXGIDevice1> dxgiDevice;
		DXUtil::ThrowIfFailed(m_d3dDevice.As(&dxgiDevice));

		// Identify the physical adapter (GPU or card) this device is running on.
		ComPtr<IDXGIAdapter> dxgiAdapter;
		DXUtil::ThrowIfFailed(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()));

		// And obtain the factory object that created it.
		ComPtr<IDXGIFactory2> dxgiFactory;
		DXUtil::ThrowIfFailed(dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.GetAddressOf())));

		// Create a descriptor for the swap chain.
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = backBufferWidth;
		swapChainDesc.Height = backBufferHeight;
		swapChainDesc.Format = backBufferFormat;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = backBufferCount;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
		fsSwapChainDesc.Windowed = TRUE;

		// Create a SwapChain from a Win32 window.
		DXUtil::ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
			m_d3dDevice.Get(),
			m_window,
			&swapChainDesc,
			&fsSwapChainDesc,
			nullptr,
			m_swapChain.ReleaseAndGetAddressOf()
		));

		// This template does not support exclusive fullscreen mode and prevents DXGI from responding to the ALT+ENTER shortcut.
		DXUtil::ThrowIfFailed(dxgiFactory->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER));
	}

	// Obtain the backbuffer for this window which will be the final 3D rendertarget.
	ComPtr<ID3D11Texture2D> backBuffer;
	DXUtil::ThrowIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())));

	// Create a view interface on the rendertarget to use on bind.
	DXUtil::ThrowIfFailed(m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTargetView.ReleaseAndGetAddressOf()));

	// Allocate a 2-D surface as the depth/stencil buffer and
	// create a DepthStencil view on this surface to use on bind.
	CD3D11_TEXTURE2D_DESC depthStencilDesc(depthBufferFormat, backBufferWidth, backBufferHeight, 1, 1, D3D11_BIND_DEPTH_STENCIL);

	ComPtr<ID3D11Texture2D> depthStencil;
	DXUtil::ThrowIfFailed(m_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, depthStencil.GetAddressOf()));

	CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
	DXUtil::ThrowIfFailed(m_d3dDevice->CreateDepthStencilView(depthStencil.Get(), &depthStencilViewDesc, m_depthStencilView.ReleaseAndGetAddressOf()));

	// TODO: Initialize windows-size dependent objects here.
}

void VideoDevice::CreateBlendState()
{
	D3D11_BLEND_DESC descBlend;
	ZeroMemory(&descBlend, sizeof(descBlend));
	descBlend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	descBlend.RenderTarget[0].BlendEnable = false;
	descBlend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	descBlend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	descBlend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	descBlend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	descBlend.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	descBlend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;	
	
	m_d3dDevice->CreateBlendState(&descBlend, m_blendState.ReleaseAndGetAddressOf());
}

void VideoDevice::_Update()
{
	float elapsedTime = float(m_timer.GetElapsedSeconds());

	// TODO: Add your game logic here.
	elapsedTime;
}

void VideoDevice::_Clear()
{
	// Clear the views.
	m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), Colors::Black);
	m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

	// Set the viewport.
	CD3D11_VIEWPORT viewport(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height));
	m_d3dContext->RSSetViewports(1, &viewport);
}

void VideoDevice::_Present()
{
	HRESULT hr = m_swapChain->Present(1, 0);

	// If the device was reset we must completely reinitialize the renderer.
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		OnDeviceLost();
	}
	else
	{
		DXUtil::ThrowIfFailed(hr);
	}
}

void VideoDevice::OnDeviceLost()
{
	LOG(INFO) << format_string("[VideoDevice::OnDeviceLost]");

	_Recreate(0);

	LOG(INFO) << format_string("[VideoDevice::OnDeviceLost] finish");
}
