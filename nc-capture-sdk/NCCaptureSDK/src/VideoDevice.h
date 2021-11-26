#pragma once

#include "common.h"
#include <stdint.h>
#include <string>
#include <windows.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <SpriteBatch.h>
#include "Singleton.h"
#include "DXUtil.h"
#include <wrl.h>
#include <DirectXTex.h>
#include <PrimitiveBatch.h>
#include <VertexTypes.h>
#include <Effects.h>
#include <CommonStates.h>
#include <mutex>

class Texture2D;
class VideoDevice : public NCCaptureSDK::Singleton<VideoDevice>
{
public:
	VideoDevice();
	~VideoDevice();

	void		RegisterCreatedCallback(DEVICE_CREATED_CALLBACK_FUNC callback, void* user_data);

	bool		Create(HWND hwnd, unsigned int width, unsigned int height);
	void		Resize(int width, int height);
	void		Destroy();

	void		Update();
	void		BeginRender();
	void		EndRender();

	bool		RecreateNextAdapter();

	Texture2D*	OpenSharedResource(HANDLE handle);

	const CapturedImage*	GetFrameBufferImage();

	void		DrawRect(const FRECT& dest, uint32_t color=0xFFFFFFFF);
	void		DrawRectInGame(const SIZE& src, const FRECT& dest, uint32_t color = 0xFFFFFFFF);

	// Messages
	void		OnActivated();
	void		OnDeactivated();
	void		OnSuspending();
	void		OnResuming();
	void		OnResizing(int width, int height);

	ID3D11Device1*			GetDevice() { return m_d3dDevice.Get(); }
	ID3D11DeviceContext1*	GetDeviceContext() { return m_d3dContext.Get(); }

	// spritebatch
	void					DrawSpriteFull(ID3D11ShaderResourceView* srv, const SIZE& srcSize, bool flip = false, DirectX::FXMVECTOR color = DirectX::Colors::White);
	void					DrawSpriteInGame(ID3D11ShaderResourceView* srv, const SIZE& gameSize, const FRECT& dest, DirectX::FXMVECTOR color = DirectX::Colors::White);
	void					DrawSprite(ID3D11ShaderResourceView* srv, const DirectX::XMFLOAT2& pos, DirectX::FXMVECTOR color = DirectX::Colors::White);
	void					DrawSprite(ID3D11ShaderResourceView* srv, const RECT& dest, bool flip = false, DirectX::FXMVECTOR color = DirectX::Colors::White);

private:
	bool					_Create(int adapter_index = 0);
	bool					_Recreate(int adapter_index);
	bool					CreateDevice(int adapter_index = 0);
	void					CreateResources();
	void					CreateBlendState();

	void					_Update();
	void					_Render();

	void					_Clear();
	void					_Present();

	void					OnDeviceLost();

	HWND					m_window;
	int						m_width;
	int						m_height;

	bool					m_bInit = false;
	std::mutex				m_render_mutex;
	int						m_current_adapter_index = 0;


	D3D_FEATURE_LEVEL		m_featureLevel;
	
	Microsoft::WRL::ComPtr<ID3D11Device1>			m_d3dDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext1>    m_d3dContext;

	Microsoft::WRL::ComPtr<IDXGISwapChain1>         m_swapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_renderTargetView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  m_depthStencilView;

	Microsoft::WRL::ComPtr<ID3D11BlendState>		m_blendState;

	std::unique_ptr<DirectX::SpriteBatch>			m_spriteBatch;
	DXUtil::StepTimer								m_timer;
	DirectX::ScratchImage							m_scratch_image;
	CapturedImage									m_image;

	std::unique_ptr<DirectX::CommonStates>                                  m_states;
	std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>  m_primitiveBatch;
	std::unique_ptr<DirectX::BasicEffect>                                   m_batchEffect;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>                               m_batchInputLayout;

	DEVICE_CREATED_CALLBACK_FUNC m_created_callback = nullptr;
	void* m_callback_user_data = nullptr;
};

