#pragma once

#include "../src/CommunityLiveCoreConfig.h"
#include "api/video_codecs/video_encoder.h"
#include "modules/video_coding/codecs/h264/include/h264.h"
#include "modules\video_coding\codecs\h264\h264_encoder_impl.h"


namespace webrtc {

class OpenH264Encoder : public VideoEncoder 
{
public:
	explicit OpenH264Encoder(const cricket::VideoCodec& codec);

	virtual ~OpenH264Encoder();
	virtual int32_t InitEncode(const VideoCodec* codec_settings,
								int32_t number_of_cores,
								size_t max_payload_size) override;

	virtual int32_t RegisterEncodeCompleteCallback(
		webrtc::EncodedImageCallback* callback) override;

	virtual int32_t Release() override;

	virtual int32_t Encode(const VideoFrame& frame, const std::vector<VideoFrameType>* video_frame_types) override;

	virtual void SetRates(const RateControlParameters& parameters) override;

	virtual void OnPacketLossRateUpdate(float packet_loss_rate);

	virtual void OnRttUpdate(int64_t rtt_ms);

	virtual void OnLossNotification(const LossNotification& loss_notification);

	virtual EncoderInfo GetEncoderInfo() const;

private:
	std::unique_ptr<H264Encoder> encoder = nullptr;
	std::string encoder_name_;

	uint32_t m_bitrate;
	double m_fps;
};

}  // namespace webrtc
