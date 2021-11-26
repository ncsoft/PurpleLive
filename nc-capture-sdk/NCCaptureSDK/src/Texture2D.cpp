#include "pch.h"
#include "LockObject.h"
#include "Texture2D.h"
#include "VideoDevice.h"
#include "easylogging++.h"

Texture2D::Texture2D()
{
	
}

Texture2D::~Texture2D()
{
	LOG(DEBUG) << format_string("[NCCapture::Texture2D::~Texture2D]");
	Destroy();
	LOG(DEBUG) << format_string("[NCCapture::Texture2D::~Texture2D] finish");
}

bool Texture2D::CreateRenderTarget(ID3D11Device1* device, uint32_t width, uint32_t height, DXGI_FORMAT format, bool isTarget/*=false*/)
{
	m_width = width;
	m_height = height;

	HRESULT result;
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;

	// Initialize the render target texture description.
	ZeroMemory(&m_textureDesc, sizeof(m_textureDesc));

	// Setup the render target texture description.
	m_textureDesc.Width = width;
	m_textureDesc.Height = height;
	m_textureDesc.MipLevels = 1;
	m_textureDesc.ArraySize = 1;
	m_textureDesc.Format = format;// DXGI_FORMAT_R8G8B8A8_UNORM;
	m_textureDesc.SampleDesc.Count = 1;
	m_textureDesc.Usage = D3D11_USAGE_DEFAULT;
	m_textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	m_textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
	m_textureDesc.MiscFlags = 0;

	// Create the render target texture.
	LOG(DEBUG) << format_string("[NCCapture::Texture2D::CreateRenderTarget] device(0x%x) CreateTexture2D", device);
	result = device->CreateTexture2D(&m_textureDesc, NULL, &m_renderTargetTexture);
	if (FAILED(result))
	{
		return false;
	}

	// Setup the description of the render target view.
	renderTargetViewDesc.Format = format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;

	// Create the render target view.
	result = device->CreateRenderTargetView(m_renderTargetTexture, &renderTargetViewDesc, &m_renderTargetView);
	if (FAILED(result))
	{
		return false;
	}

	// Setup the description of the shader resource view.
	shaderResourceViewDesc.Format = m_textureDesc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	// Create the shader resource view.
	result = device->CreateShaderResourceView(m_renderTargetTexture, &shaderResourceViewDesc, &m_renderTargetShaderResourceView);
	if (FAILED(result))
	{
		return false;
	}

	// create sprite
	m_spriteBatch = std::make_unique<DirectX::SpriteBatch>(VideoDevice::Instance()->GetDeviceContext());

	return true;
}

void Texture2D::Destroy()
{
	if (m_shaderResourceView)
	{
		m_shaderResourceView->Release();
		m_shaderResourceView = 0;
	}

	if (m_renderTargetView)
	{
		m_renderTargetView->Release();
		m_renderTargetView = 0;
	}

	if (m_renderTargetTexture)
	{
		m_renderTargetTexture->Release();
		m_renderTargetTexture = 0;
	}

	return;
}

bool Texture2D::CreateAsSharedResource(ID3D11Device1* device, HANDLE handle)
{
	return OpenSharedResource(device, handle);
}

bool Texture2D::OpenSharedResource(ID3D11Device1* device, HANDLE handle)
{
	LOG(DEBUG) << format_string("[NCCapture::Texture2D::OpenSharedResource] device(0x%x) handle(0x%x)", device, handle);

	std::lock_guard<std::mutex> guard(g_mtx_lock_video);
	// gc->shtex_data->tex_handle
	HRESULT hr = device->OpenSharedResource(handle, __uuidof(ID3D11Texture2D), (void**)(&m_texture));
	if (FAILED(hr))
	{
		LOG(ERROR) << format_string("[NCCapture::Texture2D::OpenSharedResource] Failed to OpenSharedResource(0x%x)", hr);
		return false;
	}

	D3D11_TEXTURE2D_DESC desc;
	m_texture->GetDesc(&desc);

	memset(&m_resourceDesc, 0, sizeof(m_resourceDesc));
	m_resourceDesc.Format = desc.Format;
	m_resourceDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	m_resourceDesc.Texture2D.MipLevels = 1;

	hr = device->CreateShaderResourceView(m_texture, &m_resourceDesc, &m_shaderResourceView);
	if (FAILED(hr))
	{
		LOG(ERROR) << format_string("[NCCapture::Texture2D::OpenSharedResource] Failed to CreateShaderResourceView(0x%x)", hr);
		return false;
	}

	m_width = desc.Width;
	m_height = desc.Height;

//	CreateRenderTarget(device, desc.Width, desc.Height, DXGI_FORMAT_R8G8B8A8_UNORM);

	return true;
}
#ifndef IID_GRAPHICS_PPV_ARGS
#define IID_GRAPHICS_PPV_ARGS(x) IID_PPV_ARGS(x)
#endif

#include <wrl.h>
#include <DirectXTex.h>
using namespace DirectX;
using namespace Microsoft::WRL;

namespace
{
	HRESULT Capture(
		_In_ ID3D11DeviceContext* pContext,
		_In_ ID3D11Resource* pSource,
		const TexMetadata& metadata,
		const ScratchImage& result) noexcept
	{
		if (!pContext || !pSource || !result.GetPixels())
			return E_POINTER;

		D3D11_MAPPED_SUBRESOURCE mapped;
		HRESULT hr = pContext->Map(pSource, 0, D3D11_MAP_READ, 0, &mapped);
		if (FAILED(hr))
			return hr;
		pContext->Unmap(pSource, 0);

		return S_OK;
	}
/*
		//--- 1D or 2D texture --------------------------------------------------------
		assert(metadata.depth == 1);

		for (size_t item = 0; item < metadata.arraySize; ++item)
		{
			size_t height = metadata.height;

			for (size_t level = 0; level < metadata.mipLevels; ++level)
			{
				UINT dindex = D3D11CalcSubresource(static_cast<UINT>(level), static_cast<UINT>(item), static_cast<UINT>(metadata.mipLevels));

				D3D11_MAPPED_SUBRESOURCE mapped;
				HRESULT hr = pContext->Map(pSource, dindex, D3D11_MAP_READ, 0, &mapped);
				if (FAILED(hr))
					return hr;

				const Image* img = result.GetImage(level, item, 0);
				if (!img)
				{
					pContext->Unmap(pSource, dindex);
					return E_FAIL;
				}

				if (!img->pixels)
				{
					pContext->Unmap(pSource, dindex);
					return E_POINTER;
				}

				size_t lines = ComputeScanlines(metadata.format, height);
				if (!lines)
				{
					pContext->Unmap(pSource, dindex);
					return E_UNEXPECTED;
				}

				auto sptr = static_cast<const uint8_t*>(mapped.pData);
				uint8_t* dptr = img->pixels;
				for (size_t h = 0; h < lines; ++h)
				{
					size_t msize = std::min<size_t>(img->rowPitch, mapped.RowPitch);
					memcpy_s(dptr, img->rowPitch, sptr, msize);
					sptr += mapped.RowPitch;
					dptr += img->rowPitch;
				}

				pContext->Unmap(pSource, dindex);

				if (height > 1)
					height >>= 1;
			}
		}

		return S_OK;
	}
*/
}

HRESULT CaptureTextureEx(
	ID3D11Device* pDevice,
	ID3D11DeviceContext* pContext,
	ID3D11Resource* pSource) noexcept
{
	if (!pDevice || !pContext || !pSource)
		return E_INVALIDARG;

	D3D11_RESOURCE_DIMENSION resType = D3D11_RESOURCE_DIMENSION_UNKNOWN;
	pSource->GetType(&resType);

	HRESULT hr = E_UNEXPECTED;

	switch (resType)
	{
	case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
	{
		ComPtr<ID3D11Texture2D> pTexture;
		hr = pSource->QueryInterface(IID_GRAPHICS_PPV_ARGS(pTexture.GetAddressOf()));
		if (FAILED(hr))
			break;

		assert(pTexture);

		D3D11_TEXTURE2D_DESC desc;
		pTexture->GetDesc(&desc);

		ComPtr<ID3D11Texture2D> pStaging;

		desc.BindFlags = 0;
		desc.MiscFlags &= D3D11_RESOURCE_MISC_TEXTURECUBE;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		desc.Usage = D3D11_USAGE_STAGING;

		LOG(DEBUG) << format_string("[NCCapture::Texture2D::CaptureTextureEx] device(0x%x) CreateTexture2D", pDevice);
		hr = pDevice->CreateTexture2D(&desc, nullptr, &pStaging);
		if (FAILED(hr))
			break;

		assert(pStaging);

		pContext->CopyResource(pStaging.Get(), pSource);

		TexMetadata mdata;
		mdata.width = desc.Width;
		mdata.height = desc.Height;
		mdata.depth = 1;
		mdata.arraySize = desc.ArraySize;
		mdata.mipLevels = desc.MipLevels;
		mdata.miscFlags = (desc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) ? TEX_MISC_TEXTURECUBE : 0u;
		mdata.miscFlags2 = 0;
		mdata.format = desc.Format;
		mdata.dimension = TEX_DIMENSION_TEXTURE2D;

//		hr = result.Initialize(mdata);
		if (FAILED(hr))
			break;

//		hr = Capture(pContext, pStaging.Get(), mdata, result);

		D3D11_MAPPED_SUBRESOURCE mapped;
		hr = pContext->Map(pStaging.Get(), 0, D3D11_MAP_READ, 0, &mapped);
		pContext->Unmap(pStaging.Get(), 0);

	}
	break;
	default:
		hr = E_FAIL;
		break;
	}

	if (FAILED(hr))
	{
		return hr;
	}

	return S_OK;
}
const DirectX::ScratchImage* Texture2D::GetImage()
{
	RenderToTexture();

	HRESULT hr = DirectX::CaptureTexture(VideoDevice::Instance()->GetDevice(), VideoDevice::Instance()->GetDeviceContext(), m_renderTargetTexture, m_image);
	if (FAILED(hr))
	{
		LOG(ERROR) << "Failed to CaptureTexture : " << hr;
		return nullptr;
	}

	return &m_image;
}

const ID3D11Texture2D* Texture2D::GetTexture()
{
	return m_texture;
/*
	RenderToTexture();

	return m_renderTargetTexture;
*/
}

inline int GetPitch(int nBitCount, int nPixelWidth)
{
	return (((nPixelWidth * nBitCount + 31) & ~31) >> 3);
}

void SaveBitmapToFile(const char* filename, DirectX::ScratchImage* si)
{
	const DirectX::Image* pImage = si->GetImages();

	if (pImage == nullptr)
		return;
	int destPitch = GetPitch(24, (int)pImage->width);
	int srcPitch = (int)pImage->rowPitch;// GetPitch(32, pImage->width);

	BITMAPINFO bi;
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = (LONG)pImage->width;
	bi.bmiHeader.biHeight = (LONG)pImage->height;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 24;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biSizeImage = (DWORD)(destPitch * pImage->height);
	bi.bmiHeader.biXPelsPerMeter = 0;
	bi.bmiHeader.biYPelsPerMeter = 0;
	bi.bmiHeader.biClrImportant = 0;
	bi.bmiHeader.biClrUsed = 0;

	// DIB 파일의 헤더 내용을 구성한다.
	BITMAPFILEHEADER bfh;
	ZeroMemory(&bfh, sizeof(BITMAPFILEHEADER));
	bfh.bfType = *(WORD*)"BM";

	bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bfh.bfSize = bi.bmiHeader.biSizeImage + bfh.bfOffBits;

	FILE* file = fopen(filename, "wb");

	if (!file)
	{
		printf("Could not write file\n");
		fclose(file);
		return;
	}

	/*Write headers*/
	fwrite(&bfh, 1, sizeof(BITMAPFILEHEADER), file);
	fwrite(&bi, 1, sizeof(BITMAPINFOHEADER), file);

	uint8_t* pPixels = pImage->pixels;
	uint8_t* pBits = new uint8_t[bi.bmiHeader.biSizeImage];

	int dest_y;
	for (int y = bi.bmiHeader.biHeight - 1; y >= 0; y--)
		//	for (int y = 0; y < bi.bmiHeader.biHeight; y++)
	{
		for (int x = 0; x < bi.bmiHeader.biWidth; x++)
		{
			dest_y = bi.bmiHeader.biHeight - (y + 1);
			pBits[dest_y*destPitch + (x * 3) + 2] = pPixels[y * srcPitch + (x * 4) + 0];	// R
			pBits[dest_y*destPitch + (x * 3) + 1] = pPixels[y * srcPitch + (x * 4) + 1];	// G
			pBits[dest_y*destPitch + (x * 3) + 0] = pPixels[y * srcPitch + (x * 4) + 2];	// B

/*			unsigned char r = pPixels[y * srcPitch + (x * 4) + 0];
			unsigned char g = pPixels[y * srcPitch + (x * 4) + 1];
			unsigned char b = pPixels[y * srcPitch + (x * 4) + 2];
			unsigned char a = pPixels[y * srcPitch + (x * 4) + 3];
			LOG(DEBUG) << format_string("[NCCapture::Texture2D::SaveBitmapToFile] [%d %d %d %d]", r, g, b, a);
*/
		}
	}

	fwrite(pBits, bi.bmiHeader.biSizeImage, 1, file);
	delete[] pBits;

	fclose(file);
}

const ID3D11ShaderResourceView * Texture2D::GetSRV()
{
/*
	RenderToTexture();

	return m_renderTargetShaderResourceView;
*/
	return m_shaderResourceView;
}
#include <SimpleMath.h>

void Texture2D::RenderToTexture()
{
	D3D11_VIEWPORT viewport;
	viewport.Width = static_cast<float>(m_width);
	viewport.Height = static_cast<float>(m_height);
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// bind viewport
	VideoDevice::Instance()->GetDeviceContext()->RSSetViewports(1, &viewport);

	VideoDevice::Instance()->GetDeviceContext()->ClearRenderTargetView(m_renderTargetView, DirectX::Colors::Black);

	// set render target
	VideoDevice::Instance()->GetDeviceContext()->OMSetRenderTargets(1, &m_renderTargetView, nullptr);

	m_spriteBatch->Begin();
	m_spriteBatch->Draw(m_shaderResourceView, DirectX::SimpleMath::Vector2(0, 0));
	m_spriteBatch->End();
}