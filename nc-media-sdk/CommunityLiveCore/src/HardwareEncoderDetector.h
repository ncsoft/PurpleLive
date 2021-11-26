#pragma once

#include "common.h"
#include <string>

enum eGPU_Vender
{
	// (주의) 외장 그래픽 카드경우 아래 vendor id가 맞지만 
	// 노트북과 같은 메인보드에 내장된 그래픽카드의 경우 vendor id가 변경 됨
	INTEL_VENDOR_ID = 0x8086,
	MICROSOFT_VENDOR_ID = 0x1414,
	NVIDIA_VENDOR_ID = 0x10de,
	AMD_VENDOR_ID = 0x1002
};

struct VideoAdapterInfo {
	unsigned int	vendorID;
	std::wstring		disc;
};

// This class is exported from the dll
class HardwareEncoderDetector {
public:
	HardwareEncoderDetector(void);
	
	static int detect();
	static bool is_supported_nvidea();
	static bool is_supported_amf();
	static bool is_supported_qsv();
};