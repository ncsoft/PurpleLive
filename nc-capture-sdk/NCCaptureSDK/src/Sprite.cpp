#include "Sprite.h"
#include "VideoDevice.h"
#include "easylogging++.h"
#include "DXUtil.h"
#include <WICTextureLoader.h>

GameCaptureSprite::GameCaptureSprite()
{
//	LOG(DEBUG) << format_string("[Sprite::Sprite]");
}

GameCaptureSprite::~GameCaptureSprite()
{
//	LOG(DEBUG) << format_string("[Sprite::~Sprite]");
}
/*
bool GameCaptureSprite::Create(int width, int height)
{
	HRESULT hr;

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = width;
	desc.Height = height;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

	hr = VideoDevice::Instance()->GetDevice()->CreateTexture2D(&desc, nullptr, m_texture&);
	if (FAILED(hr))
	{
		LOG(ERROR) << "Failed to Create Texture2D " << hr;
		return false;
	}

	return true;
}
*/
/*
bool GameCaptureSprite::Create(ID3D11Texture2D * texture)
{
	HRESULT hr;
	D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
	srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MostDetailedMip = 0;
	srv_desc.Texture2D.MipLevels = 1;

	hr = VideoDevice::Instance()->GetDevice()->CreateShaderResourceView(texture, &srv_desc, &m_srv);
	if (FAILED(hr))
	{
		LOG(ERROR) << "Failed to CreateShaderResourceView : " << hr;
		return false;
	}
	
	if (m_srv)
	{
		D3D11_TEXTURE2D_DESC desc;
		texture->GetDesc(&desc);

		m_textureWidth = desc.Width;
		m_textureHeight = desc.Height;

		m_textureSize.x = 0.f;
		m_textureSize.y = float(desc.Height);

		m_origin.x = desc.Width / 2.f;
		m_origin.y = 0.f;

		return true;
	}
	else
		return false;

	return false;
}

bool GameCaptureSprite::Create(CapturedImage* pImage)
{
	if (pImage == nullptr)
		return false;
	D3D11_TEXTURE2D_DESC tdesc;
	ZeroMemory(&tdesc, sizeof(tdesc));
	D3D11_SUBRESOURCE_DATA sr_data;
	ZeroMemory(&sr_data, sizeof(sr_data));
//	ID3D11Texture2D* tex = 0;
	D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
	ZeroMemory(&srv_desc, sizeof(srv_desc));

	HRESULT hr = 0;
	// setting up D3D11_SUBRESOURCE_DATA 
	sr_data.pSysMem = (void *)pImage->pixels;
	sr_data.SysMemPitch = pImage->rowPitch;// w * bpp;
	sr_data.SysMemSlicePitch = pImage->slicePitch;// w * h*bpp; // Not needed since this is a 2d texture

	// setting up D3D11_TEXTURE2D_DESC 
	tdesc.Width = pImage->width;
	tdesc.Height = pImage->height;
	tdesc.MipLevels = 1;
	tdesc.ArraySize = 1;
	tdesc.SampleDesc.Count = 1;
	tdesc.SampleDesc.Quality = 0;
	tdesc.Usage = D3D11_USAGE_STAGING;
	tdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	tdesc.BindFlags = 0;
	tdesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	tdesc.MiscFlags = 0;

	// checking inputs
	if (VideoDevice::Instance()->GetDevice()->CreateTexture2D(&tdesc, &sr_data, NULL) == S_FALSE)
		LOG(DEBUG) << "Inputs correct";
	else
		LOG(DEBUG) << "wrong inputs";

	// create the texture
	hr = VideoDevice::Instance()->GetDevice()->CreateTexture2D(&tdesc, &sr_data, m_texture.GetAddressOf());
	if (FAILED(hr))
	{
		LOG(ERROR) << "Failed to CreateTexture2D " << hr;
	}
//	else
//		std::cout << "Success" << std::endl;


	// setup the Shader Resource Desc.
	srv_desc.Format = tdesc.Format;
	srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MostDetailedMip = 0;
	srv_desc.Texture2D.MipLevels = 1;

	hr = VideoDevice::Instance()->GetDevice()->CreateShaderResourceView(m_texture.Get(), &srv_desc, &m_srv);
	if (SUCCEEDED(hr))
	{
		return true;
	}

	LOG(ERROR) << "Failed to CreateShaderResourceView " << hr;

	return false;
}
*/
void GameCaptureSprite::UpdateTexture(ID3D11Texture2D * texture)
{
	VideoDevice::Instance()->GetDeviceContext()->CopyResource(m_texture.Get(), texture);
	return;
}
/*
void Sprite::UpdateTexture(DirectX::Image * pImage)
{
	D3D11_MAPPED_SUBRESOURCE mr;
	
	D3D11_MAPPED_SUBRESOURCE map = {};
	HRESULT hr;

	hr = VideoDevice::Instance()->GetDeviceContext()->Map(data.copy_surfaces[idx], 0, D3D11_MAP_READ, 0, &map);
	if (FAILED(hr)) 
	{
		LOG(ERROR) << "Failed to Map " << hr;
	}

	data.pitch = map.RowPitch;
	data.context->Unmap(data.copy_surfaces[idx], 0);

	VideoDevice::Instance()->GetDeviceContext()->Map(m_srv,)
}
*/
bool GameCaptureSprite::Create(ID3D11ShaderResourceView* srv, bool flip)
{
	m_srv = srv;
	m_flip = flip;

	if (m_srv)
	{
		Microsoft::WRL::ComPtr<ID3D11Resource> resource;
		srv->GetResource(resource.GetAddressOf());

		D3D11_RESOURCE_DIMENSION dim;
		resource->GetType(&dim);

		if (dim != D3D11_RESOURCE_DIMENSION_TEXTURE2D)
			throw std::exception("sprite expects a Texture2D");

		Microsoft::WRL::ComPtr<ID3D11Texture2D> tex2D;
		resource.As(&tex2D);

		D3D11_TEXTURE2D_DESC desc;
		tex2D->GetDesc(&desc);

		m_textureWidth = desc.Width;
		m_textureHeight = desc.Height;

		m_textureSize.x = 0.f;
		m_textureSize.y = float(desc.Height);

		m_origin.x = desc.Width / 2.f;
		m_origin.y = 0.f;

		LOG(DEBUG) << format_string("[GameCaptureSprite::Create] success");

		return true;
	}
	else
	{
		LOG(DEBUG) << format_string("[GameCaptureSprite::Create] failed");
		return false;
	}
}

void GameCaptureSprite::DrawFull(FXMVECTOR color)
{
	if (m_srv == nullptr)
		return;

	SIZE srcSize = { m_textureWidth, m_textureHeight };
	VideoDevice::Instance()->DrawSpriteFull(m_srv.Get(), srcSize, m_flip, color);
}

void GameCaptureSprite::Draw(int x, int y, FXMVECTOR color)
{
	if (m_srv == nullptr)
		return;

	m_screenPos.x = (float)x;
	m_screenPos.y = (float)y;

	VideoDevice::Instance()->DrawSprite(m_srv.Get(), m_screenPos, color);
}

void GameCaptureSprite::Draw(const RECT& dest, FXMVECTOR color)
{
	if (m_srv == nullptr)
		return;

	VideoDevice::Instance()->DrawSprite(m_srv.Get(), dest, m_flip, color);
}

GameSprite::GameSprite()
{
}

GameSprite::~GameSprite()
{
}

bool GameSprite::Create(const wchar_t* filename)
{
	HRESULT hr = CreateWICTextureFromFileEx(VideoDevice::Instance()->GetDevice(), filename, 0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, D3D11_RESOURCE_MISC_SHARED, WIC_LOADER_IGNORE_SRGB, nullptr, m_srv.ReleaseAndGetAddressOf());
	
	if (SUCCEEDED(hr))
	{
		return true;
	}
	else
	{
		std::wstring wstr = filename;
		std::string str(wstr.begin(), wstr.end());
		LOG(ERROR) << format_string("[GameSprite::Create] failed to load %s file", str.c_str());
		return false;
	}
}

void GameSprite::Draw(const SIZE& gameSize, const FRECT& dest, DirectX::FXMVECTOR color)
{
	VideoDevice::Instance()->DrawSpriteInGame(m_srv.Get(), gameSize, dest, color);
}
