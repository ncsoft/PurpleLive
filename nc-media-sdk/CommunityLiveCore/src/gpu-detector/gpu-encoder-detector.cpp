#include "gpu-encoder-detector.h"
#include <dxgi.h>
#include <d3d11.h>
#ifdef _WIN32_WINNT_WIN10
#include <d3d11_3.h>
#endif

#include "../nvcodec/nvenc.h"
#include "../amf/amf.hpp"
#include "../amf/api-base.hpp"
#include "../qsv/QSV_Encoder.h"

#include <iostream>
#include "../util.h"

bool check_enable_encoder_amd() {
	try {
		Plugin::AMD::AMF::Initialize();
	}
	catch (...) {
		ELOGE << "[gpu-encoder-detector.cpp!::check_enable_encoder_amd] [error] Exception during AMF initialization.";
		return false;
	}

	// Initialize Graphics APIs
	try {
		Plugin::API::InitializeAPIs();
	}
	catch (...) {
		ELOGE << "[gpu-encoder-detector.cpp!::check_enable_encoder_amd] [error] Exception during AMF API initialization.";
		Plugin::AMD::AMF::Finalize();
		return false;
	}

	Plugin::AMD::AMF::Finalize();
	return true;
}

bool check_enable_encoder_nvidia() {
	void* adr = nvenc_info.create();

	encoder_config nvenc_config;
	nvenc_config.codec = "h264";
	nvenc_config.format = DXGI_FORMAT_B8G8R8A8_UNORM;
	nvenc_config.width = 1024;
	nvenc_config.height = 1024;
	nvenc_config.framerate = 30;
	nvenc_config.gop = 1;
	nvenc_config.bitrate = 2000;

	try { nvenc_info.init(adr, &nvenc_config); }
	catch (...) {
		return false;
	}

	nvenc_info.destroy(&adr);
	return true;
}

bool check_enable_encoder_qsv() {
	qsv_t*	m_qsv = nullptr;
	qsv_param_t params;

	params.nTargetUsage = MFX_TARGETUSAGE_BALANCED;
	params.nWidth = 1280;
	params.nHeight = 720;
	params.nAsyncDepth = 1;	// for remove delay from input to output
	params.nFpsNum = 30;
	params.nFpsDen = 1;
	params.nTargetBitRate = 2500;
	params.nMaxBitRate = 2500;
	params.nCodecProfile = MFX_PROFILE_AVC_HIGH;
	params.nAccuracy = 1000;
	params.nConvergence = 1;
	params.nQPI = 32;
	params.nQPP = 28;
	params.nQPB = 31;
	params.nLADEPTH = 15;
	params.nbFrames = 0;
	params.nICQQuality = 23;
	params.bMBBRC = true;
	params.nRateControl = MFX_RATECONTROL_AVBR;

	m_qsv = qsv_encoder_open(&params);

	if(m_qsv == nullptr) return false;

	qsv_encoder_close(m_qsv);

	return true;
}

int detect(std::vector<VideoAdapterInfo> &infos)
{
	int result = HES_NONE;

	int adapterIndex = 0;

	while (adapterIndex < infos.size())
	{
		const std::wstring gpuDescription = infos[adapterIndex].disc;
		unsigned int uiVendorID = infos[adapterIndex].vendorID;

		if (uiVendorID == eGPU_Vender::INTEL_VENDOR_ID || gpuDescription.find(L"Intel") != std::wstring::npos)
		{
			if (check_enable_encoder_qsv())
			{
				ELOGI << format_string("[QSV] Detected and Encode(1)");
				result |= HES_QSV;
			}
			else
			{
				ELOGI << format_string("[QSV] Detected and Encode(0)");
			}
		}
		else if (uiVendorID == eGPU_Vender::NVIDIA_VENDOR_ID || gpuDescription.find(L"NVIDIA") != std::wstring::npos)
		{
			if (check_enable_encoder_nvidia())
			{
				ELOGI << format_string("[NVIDIA] Detected and Encode(1)");
				result |= HES_NVIDIA;
			}
			else
			{
				ELOGI << format_string("[NVIDIA] Detected and Encode(0)");
			}
		}
		else if (uiVendorID == eGPU_Vender::AMD_VENDOR_ID || gpuDescription.find(L"AMD") != std::wstring::npos)
		{
			if (check_enable_encoder_amd())
			{
				ELOGI << format_string("[AMF] Detected and Encode(1)");
				result |= HES_AMF;
			}
			else
			{
				ELOGI << format_string("[AMF] Detected and Encode(0)");
			}
		}
		else if (uiVendorID == eGPU_Vender::MICROSOFT_VENDOR_ID || gpuDescription.find(L"Microsoft") != std::wstring::npos)
		{
			ELOGD << format_string("[Microsoft] Detected and skip");
		}
		else 
		{

		}

		adapterIndex++;
	}

	return result;
}

