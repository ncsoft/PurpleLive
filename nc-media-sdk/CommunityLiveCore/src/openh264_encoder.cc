#include "openh264_encoder.h"
#include "easylogging++.h"
#include "../src/util.h"
#include "EncoderStatistics.h"

namespace webrtc
{

OpenH264Encoder::OpenH264Encoder(const cricket::VideoCodec& codec) {
	encoder = H264Encoder::Create(codec);
	encoder_name_ = "OpenH264Encoder";
}

OpenH264Encoder::~OpenH264Encoder() {
	
}

//WebRTC interfaces.
int32_t OpenH264Encoder::InitEncode(const webrtc::VideoCodec* codec_settings,
	int32_t number_of_cores,
	size_t max_payload_size)
{
	if(encoder==nullptr)
		return 0;

	m_bitrate = 0;
	m_fps = 0;

	g_es->InitEncode(encoder_name_);

	return encoder->InitEncode(codec_settings, number_of_cores, max_payload_size);
}

int32_t OpenH264Encoder::RegisterEncodeCompleteCallback(
	webrtc::EncodedImageCallback* callback)
{
	if(encoder==nullptr)
		return 0;

	return encoder->RegisterEncodeCompleteCallback(callback);
}

int32_t OpenH264Encoder::Release()
{
	if(encoder==nullptr)
		return 0;

	return encoder->Release();
}

int32_t OpenH264Encoder::Encode(const webrtc::VideoFrame& frame, const std::vector<webrtc::VideoFrameType>* frame_types)
{
	if(encoder==nullptr)
		return 0;

	//bool request_key_frame = ((*frame_types)[0]==VideoFrameType::kVideoFrameKey);
	//if(request_key_frame)
	//	ELOGD << format_string("[OpenH264Encoder::Encode] requested KeyFrame - kVideoFrameKey(%d)", request_key_frame);

#if USE_ENCODER_PERFORMANCE_TEST
	g_es->EncodeFrameStart();
#endif

	int32_t res = encoder->Encode(frame, frame_types);
	if(res!=WEBRTC_VIDEO_CODEC_OK)
		ELOGD << format_string("[OpenH264Encoder::Encode] error(0x%x)", res);

#if USE_ENCODER_PERFORMANCE_TEST
	g_es->EncodeFrameEnd();
#endif

	return res;
}

void OpenH264Encoder::OnPacketLossRateUpdate(float packet_loss_rate)
{
	if (packet_loss_rate>0)
		ELOGD << format_string("[%s::OnPacketLossRateUpdate] packet_loss_rate(%f)", encoder_name_.c_str(), packet_loss_rate);
}

void OpenH264Encoder::OnRttUpdate(int64_t rtt_ms)
{
	//ELOGD << format_string("[%s::OnRttUpdate] rtt_ms(%lld)", encoder_name_.c_str(), rtt_ms);
}

void OpenH264Encoder::OnLossNotification(const LossNotification& loss_notification)
{
	ELOGD << format_string("[%s::OnLossNotification] timestamp_of_last_decodable(%d) timestamp_of_last_received(%d)", encoder_name_.c_str(), loss_notification.timestamp_of_last_decodable, loss_notification.timestamp_of_last_received);
	ELOGD << format_string("[%s::OnLossNotification] dependencies_of_last_received_decodable(%d) last_received_decodable(%d)", encoder_name_.c_str(), loss_notification.dependencies_of_last_received_decodable, loss_notification.last_received_decodable);
}

VideoEncoder::EncoderInfo OpenH264Encoder::GetEncoderInfo() const
{
	EncoderInfo info = __super::GetEncoderInfo();
	info.implementation_name = encoder_name_;
	info.is_hardware_accelerated = true;
	info.supports_simulcast = false;
	return info;
}

void OpenH264Encoder::SetRates(const RateControlParameters& parameters)
{
	if(encoder==nullptr)
		return;

	bool is_update = (m_bitrate != parameters.bitrate.get_sum_bps()) || (m_fps != parameters.framerate_fps);
	if (!is_update)
	{
		m_bitrate = parameters.bitrate.get_sum_bps();
		m_fps = parameters.framerate_fps;
		ELOGD << format_string("[%s::SetRates] framerate(%lf) bitrate(%d Kbps) bandwidth(%ld Kbps) is_bw_limited(%d)", encoder_name_.c_str(), parameters.framerate_fps, parameters.bitrate.get_sum_kbps(), parameters.bandwidth_allocation.kbps(), parameters.bitrate.is_bw_limited());
	}

	return encoder->SetRates(parameters);
}

} // namespace webrtc
