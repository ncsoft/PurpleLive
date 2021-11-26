#pragma once

#include "api/video_codecs/video_encoder.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "rtc_base/async_invoker.h"
#include "media/engine/internal_encoder_factory.h"
#include "api/video_codecs/sdp_video_format.h"
#include "common_video/h264/h264_bitstream_parser.h"

#include "nv_encoder.h"
#include "amf_encoder.h"
#include "qsv11_encoder.h"
#include "openh264_encoder.h"
#include "common.h"

namespace webrtc {

class PurpleVideoEncoderFactory : public VideoEncoderFactory 
{
public:
	VideoEncodingType encodeType_;
	int framerate_;

	PurpleVideoEncoderFactory(VideoEncodingType encodeType, int framerate) 
		: encodeType_(encodeType), framerate_(framerate) {}

	std::vector<webrtc::SdpVideoFormat> GetSupportedFormats()
		const override {
		std::vector<webrtc::SdpVideoFormat> video_formats;
		for (const webrtc::SdpVideoFormat& h264_format : webrtc::SupportedH264Codecs())
			video_formats.push_back(h264_format);
		return video_formats;
	}

	VideoEncoderFactory::CodecInfo QueryVideoEncoder(
		const webrtc::SdpVideoFormat& format) const override {
		CodecInfo codec_info = { false, false };
		codec_info.is_hardware_accelerated = true;
		codec_info.has_internal_source = false;
		return codec_info;
	}

	std::unique_ptr<webrtc::VideoEncoder> CreateVideoEncoder(
		const webrtc::SdpVideoFormat& format) override 
	{
		if (_stricmp(format.name.c_str(), cricket::kH264CodecName) == 0)
		{
			if (webrtc::H264Encoder::IsSupported()) 
			{
				switch(encodeType_)
				{
				case VideoEncodingType::VET_SOFTWARE:
					return absl::make_unique<webrtc::OpenH264Encoder>(cricket::VideoCodec(format));
				case VideoEncodingType::VET_HARDWARE_NVENC:
					return absl::make_unique<webrtc::NvEncoder>(cricket::VideoCodec(format), framerate_);
				case VideoEncodingType::VET_HARDWARE_AMF:
					return absl::make_unique<webrtc::AmfEncoder>(cricket::VideoCodec(format), framerate_);
				case VideoEncodingType::VET_QSV11:
					return absl::make_unique<webrtc::QsvEncoder>(cricket::VideoCodec(format), framerate_);
				default:
					return nullptr;
				}
			}
		}

		return nullptr;
	}
};

std::unique_ptr<VideoEncoderFactory> CreatePurpleVideoEncoderFactory(VideoEncodingType encodeType, int framerate)
{
	return absl::make_unique<PurpleVideoEncoderFactory>(encodeType, framerate);
}
} // namespace webrtc