#pragma once

#include <inttypes.h>
#include "Singleton.h"

constexpr int32_t kMaxWebrtcVideoEncoderFps = 60;
constexpr int32_t kMaxWebrtcVideoEncoderBitrate = 20 * 1000 * 1000;

enum class AVC_NAL_TYPE : uint8_t {
	AVC_NAL_UNKNOWN = 0,
	AVC_NAL_SLICE = 1,
	AVC_NAL_SLICE_DPA = 2,
	AVC_NAL_SLICE_DPB = 3,
	AVC_NAL_SLICE_DPC = 4,
	AVC_NAL_IDR = 5,
	AVC_NAL_SEI = 6,
	AVC_NAL_SPS = 7,
	AVC_NAL_PPS = 8,
	AVC_NAL_AUD = 9,
	AVC_NAL_FILLER = 12,
};

#ifndef _EncoderQualityPreset
#define _EncoderQualityPreset
	enum EncoderQualityPreset {
		Speed = 0,
		Balanced,
		Quality,
	};
#endif //_EncoderQualityPreset

#ifndef _EncoderRateControl
#define _EncoderRateControl
	enum class EncoderRateControl: uint8_t {
		VBR = 0,
		CBR,
	};
#endif //_EncoderRateControl

// singleton
class CommonEncoderConfig : public CommunityLiveCore::Singleton<CommonEncoderConfig>
{
public:
	EncoderQualityPreset quality_preset = EncoderQualityPreset::Speed;
	EncoderRateControl rate_control = EncoderRateControl::VBR;	
};

#define g_cec		(CommonEncoderConfig::Instance())