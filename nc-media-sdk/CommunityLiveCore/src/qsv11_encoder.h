#pragma once

#include "../src/CommunityLiveCoreConfig.h"
#include "api/video_codecs/video_encoder.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "rtc_base/async_invoker.h"
#include "media/engine/internal_encoder_factory.h"
#include "api/video_codecs/sdp_video_format.h"
#include "common_video/h264/h264_bitstream_parser.h"
#include "modules/video_coding/codecs/h264/include/h264.h"

#include "qsv/QSV_Encoder.h"

namespace webrtc {

enum class QsvQualityPreset : uint8_t {
	Speed,
	Balanced,
	Quality,
};

enum class QsvRateControl: uint8_t {
	CBR,
	VBR,
	VCM,
	CQP,
	AVBR,
	ICQ,
	LA_ICQ,
	LA_VBR,
	LA_CBR,
};

struct QsvEncoderConfig {
	int32_t width = 0;
	int32_t height = 0;
	int32_t target_bitrate = 2500 * 1000;
	int32_t max_bitrate = 2500 * 1000;
	int32_t encoder_bitrate = 2500 * 1000;
	int32_t fps_num = 30;
	int32_t fps_den = 1;
	QsvRateControl rate_control = QsvRateControl::VBR;
	QsvQualityPreset quality_preset = QsvQualityPreset::Speed;
};

class QsvEncoder : public VideoEncoder 
{
public:
	explicit QsvEncoder(const cricket::VideoCodec& codec, int framerate);

	virtual ~QsvEncoder();
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

	bool open();
	bool close();

	qsv_t*	m_qsv = nullptr;

	std::string encoder_name_;
	QsvEncoderConfig config_;
	int framerate_ = 30;
	bool first_frame_ = true;

	webrtc::EncodedImageCallback* callback_ = NULL;
	webrtc::H264BitstreamParser h264_bitstream_parser_;

	std::unique_ptr<uint8_t[]> nv12_buff_;
	size_t  frame_length_ = 0;
	uint8_t *nv12_y_plane_ = NULL;
	uint8_t *nv12_uv_plane_ = NULL;

	qsv_param_t params_;
};
}  // namespace webrtc
