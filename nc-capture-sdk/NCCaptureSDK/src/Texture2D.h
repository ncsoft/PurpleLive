#pragma once
#include <stdint.h>
#include <dxgi.h>
#include <d3d11_1.h>
#include "DirectXTex.h"
#include "SpriteBatch.h"
#include <memory>

class Texture2D
{
public:
	Texture2D();
	~Texture2D();
	
	bool							CreateAsSharedResource(ID3D11Device1* device, HANDLE handle);
	const DirectX::ScratchImage*	GetImage();
	const ID3D11Texture2D*			GetTexture();
	const ID3D11ShaderResourceView*	GetSRV();
	uint32_t						GetWidth() { return m_width; }
	uint32_t						GetHeight() { return m_height; }
	void							SetFlip(bool enable) { m_flip = enable; }
	bool							GetFlip() { return m_flip; }

private:
	void							Destroy();
	bool							OpenSharedResource(ID3D11Device1* device, HANDLE handle);
	void							RenderToTexture();
	bool							CreateRenderTarget(ID3D11Device1* device, uint32_t width, uint32_t height, DXGI_FORMAT format, bool isTarget = false);

	uint32_t						m_width;
	uint32_t						m_height;
	DXGI_FORMAT						m_format;
	D3D11_TEXTURE2D_DESC			m_textureDesc;
	bool							m_flip;

	// shared texture
	ID3D11Texture2D*				m_texture = nullptr;
	ID3D11ShaderResourceView*		m_shaderResourceView = nullptr;

	// render target texture
	ID3D11Texture2D*				m_renderTargetTexture = nullptr;
	ID3D11RenderTargetView*			m_renderTargetView = nullptr;
	ID3D11ShaderResourceView*		m_renderTargetShaderResourceView = nullptr;
	D3D11_SHADER_RESOURCE_VIEW_DESC m_resourceDesc;

	IDXGISurface1*					m_gdiSurface;
	

	DirectX::ScratchImage			m_image;
	std::unique_ptr<DirectX::SpriteBatch>	m_spriteBatch;
};
