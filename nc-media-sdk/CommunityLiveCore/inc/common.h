#pragma once
#ifdef COMMUNITYLIVECORE_EXPORTS
#define COMMUNITY_LIVE_CORE_API __declspec(dllexport)
#else
#define COMMUNITY_LIVE_CORE_API __declspec(dllimport)
#endif

#define DEFAULT_ENCODER_TYPE		VideoEncodingType::VET_SOFTWARE
#define DEFAULT_VIDEO_FRAMERATE		30

enum VideoEncodingType
{
	VET_Unknown,
	VET_SOFTWARE,
	VET_HARDWARE_NVENC,
	VET_HARDWARE_AMF,
	VET_QSV11,
};

enum HardwareEncoderSupport
{
	HES_NONE		= 0x00000000,
	HES_NVIDIA		= 0x00000001,
	HES_AMF			= 0x00000002,
	HES_QSV			= 0x00000004,
	HES_ALL			= 0x0000FFFF, 
	HES_ERROR		= 0xF0000000,
};
