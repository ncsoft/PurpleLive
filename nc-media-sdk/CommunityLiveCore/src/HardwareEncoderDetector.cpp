#include "HardwareEncoderDetector.h"
#include "gpu-detector/gpu-encoder-detector.h"
#include <iostream>
#include <list>
#include <vector>
#include <dxgi.h>

HardwareEncoderDetector::HardwareEncoderDetector()
{
    return;
}

int g_hardware_encoder_support = HES_NONE;

int HardwareEncoderDetector::detect()
{
	IDXGIAdapter * pAdapter;
	std::vector <IDXGIAdapter*> vAdapters;
	IDXGIFactory* pFactory = NULL;
	std::vector<VideoAdapterInfo> infos;

	// Create a DXGIFactory object.
	HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory);
	if (FAILED(hr))
	{
		throw std::exception("<HardwareEncoderDetector::detect()> Failed to create D3D9Ex, error code %X.", hr);
	}

	for (UINT i = 0; pFactory->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		VideoAdapterInfo info;
		DXGI_ADAPTER_DESC desc;
		pAdapter->GetDesc(&desc);
		info.vendorID = desc.VendorId;
		info.disc = desc.Description;
		infos.push_back(info);
	}

	if (pFactory)
	{
		pFactory->Release();
	}

	g_hardware_encoder_support = ::detect(infos);
	return g_hardware_encoder_support;
}

bool HardwareEncoderDetector::is_supported_nvidea()
{
	return g_hardware_encoder_support & HES_NVIDIA;
}

bool HardwareEncoderDetector::is_supported_amf()
{
	return g_hardware_encoder_support & HES_AMF;
}

bool HardwareEncoderDetector::is_supported_qsv()
{
	return g_hardware_encoder_support & HES_QSV;
}