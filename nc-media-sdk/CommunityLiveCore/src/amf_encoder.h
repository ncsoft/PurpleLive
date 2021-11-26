#pragma once

#include "../src/CommunityLiveCoreConfig.h"
#include "api/video_codecs/video_encoder.h"
#include "modules/video_coding/codecs/h264/include/h264.h"
#include "common_video/h264/h264_bitstream_parser.h"
#include "modules\video_coding\codecs\h264\h264_encoder_impl.h"
#include "amf/enc-h264.hpp"


namespace webrtc {

class AmfEncoder : public VideoEncoder 
{
public:
	explicit AmfEncoder(const cricket::VideoCodec& codec, int framerate);

	virtual ~AmfEncoder();
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
	bool	EncodeFrame(const VideoFrame& input_frame,
					std::vector<std::vector<uint8_t>>& frame_packet, bool is_key_frame=false);

	bool	IsKeyFrame(const std::vector<uint8_t>& encoded_output_buffer);
	void	InsertHeader(std::vector<uint8_t>& encoded_output_buffer);

	bool open();
	bool close();

	std::string encoder_name_;
	AmfEncoderConfig config_;
	int framerate_ = 30;
	bool first_frame_ = true;

	webrtc::EncodedImageCallback* callback_ = nullptr;
	webrtc::H264BitstreamParser h264_bitstream_parser_;

	std::unique_ptr<uint8_t[]> nv12_buff_;
	size_t  frame_length_ = 0;
	uint8_t *nv12_y_plane_ = nullptr;
	uint8_t *nv12_uv_plane_ = nullptr;

	Plugin::Interface::H264Interface* encoder = nullptr;
};

}  // namespace webrtc
