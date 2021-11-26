#pragma once

#include <memory>
#include <vector>
#include <string.h>

#include "../src/CommunityLiveCoreConfig.h"
#include "api/video_codecs/video_encoder.h"
#include "api/video/i420_buffer.h"
#include "rtc_base/async_invoker.h"
#include "common_video/h264/h264_bitstream_parser.h"
#include "modules/video_coding/codecs/h264/include/h264.h"
#include "modules/video_coding/utility/quality_scaler.h"
#include "third_party/openh264/src/codec/api/svc/codec_app_def.h"

#include <dxgi.h>
#include <d3d11.h>
#include <dxgi1_2.h>

#include "nvcodec/nvenc.h"

namespace webrtc {

class NvEncoder : public VideoEncoder {
public:
	struct LayerConfig 
	{
	int simulcast_idx = 0;
	int width = -1;
	int height = -1;
	bool sending = true;
	bool key_frame_request = false;
	float max_frame_rate = 0;
	uint32_t target_bps = 0;
	uint32_t max_bps = 0;
	bool frame_dropping_on = false;
	int key_frame_interval = 0;

	void SetStreamState(bool send_stream);
	};

public:
	explicit NvEncoder(const cricket::VideoCodec& codec, int framerate);
	~NvEncoder() override;

	int32_t InitEncode(const VideoCodec* codec_settings,
						int32_t number_of_cores,
						size_t max_payload_size) override;
	int32_t Release() override;

	int32_t RegisterEncodeCompleteCallback(
		EncodedImageCallback* callback) override;

	virtual int32_t Encode(const VideoFrame& frame, const std::vector<VideoFrameType>* video_frame_types) override;

	virtual void SetRates(const RateControlParameters& parameters) override;

	virtual void OnPacketLossRateUpdate(float packet_loss_rate);

	virtual void OnRttUpdate(int64_t rtt_ms);

	virtual void OnLossNotification(const LossNotification& loss_notification);

	virtual EncoderInfo GetEncoderInfo() const;

	// Exposed for testing.
	H264PacketizationMode PacketizationModeForTesting() const {
		return packetization_mode_;
	}

private:
	webrtc::H264BitstreamParser h264_bitstream_parser_;
	// Reports statistics with histograms.
	void ReportInit();
	void ReportError();

	bool EncodeFrame(int index,const VideoFrame& input_frame,
					std::vector<std::vector<uint8_t>>& frame_packet);

	std::string encoder_name_;

	std::unique_ptr<rtc::Thread> encode_thread_;
	std::vector<void*> nv_encoders_;
	std::vector<LayerConfig> configurations_;
	std::vector<EncodedImage> encoded_images_;

	VideoCodec codec_;
	H264PacketizationMode packetization_mode_;
	size_t max_payload_size_;
	int32_t number_of_cores_;
	EncodedImageCallback* encoded_image_callback_;

	bool has_reported_init_;
	bool has_reported_error_;
	int video_format_;
	int num_temporal_layers_;
	uint8_t tl0sync_limit_;

	std::shared_ptr<uint8_t> image_buffer_;

	uint32_t m_bitrate;
	int m_framerate;
};

}  // namespace webrtc
