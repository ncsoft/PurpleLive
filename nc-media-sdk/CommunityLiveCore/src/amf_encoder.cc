#include "api/video_codecs/video_encoder.h"
#include "modules/include/module_common_types.h"
#include "common_video/h264/h264_common.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "modules/video_coding/include/video_codec_interface.h"

#include "amf/amf-encoder.hpp"
#include "amf_encoder.h"

#include "easylogging++.h"
#include "../src/util.h"
#include "VideoEncoderUtil.h"
#include "EncoderStatistics.h"
#include "avc_util.hpp"

namespace webrtc
{

AmfEncoder::AmfEncoder(const cricket::VideoCodec& codec, int framerate)
	: framerate_(framerate)
{
	encoder_name_ = "AmfEncoder";
}

AmfEncoder::~AmfEncoder() {
	
}

//WebRTC interfaces.
int32_t AmfEncoder::InitEncode(const webrtc::VideoCodec* codec_settings,
	int32_t number_of_cores,
	size_t max_payload_size)
{
	int32_t ret = 0;
	do {
		config_.width = codec_settings->width;
		config_.height = codec_settings->height;
		
		config_.max_bitrate = std::min<int32_t>(kMaxWebrtcVideoEncoderBitrate, codec_settings->maxBitrate*1000);
		config_.target_bitrate = std::min<int32_t>(kMaxWebrtcVideoEncoderBitrate, codec_settings->maxBitrate*1000);
		if(config_.target_bitrate==0)
			config_.target_bitrate = config_.max_bitrate;

		config_.fps_num = framerate_;
		first_frame_ = true;

		switch (g_cec->quality_preset)
		{
			case EncoderQualityPreset::Speed:
				config_.quality_preset = AmfQualityPreset::Speed; 
				break;	
			case EncoderQualityPreset::Balanced:
				config_.quality_preset = AmfQualityPreset::Balanced; 
				break;	
			case EncoderQualityPreset::Quality:
				config_.quality_preset = AmfQualityPreset::Quality; 
				break;	
			default:
				config_.quality_preset = AmfQualityPreset::Speed; 
				break;	
		}

		switch (g_cec->rate_control)
		{
			case EncoderRateControl::VBR:
				config_.rate_control = AmfRateControl::VBR; 
				break;	
			case EncoderRateControl::CBR:
				config_.rate_control = AmfRateControl::CBR; 
				break;	
			default:
				config_.rate_control = AmfRateControl::VBR; 
				break;	
		}

		ELOGD << format_string("[AmfEncoder::InitEncode] video(%d %d)", config_.width, config_.height);
		ELOGD << format_string("[AmfEncoder::InitEncode] fps_num(%d) max_bitrate(%d) target_bitrate(%d)", config_.fps_num, config_.max_bitrate, config_.target_bitrate);
		ELOGD << format_string("[AmfEncoder::InitEncode] quality_preset(%d) common_rate_control(%d), rate_control(%d) ", config_.quality_preset, g_cec->rate_control, config_.rate_control);

		if (!open()) 
		{
			ret = -1;
			break;
		}
	} while (0);

	g_es->InitEncode(encoder_name_);

	return 0;
}

int32_t AmfEncoder::RegisterEncodeCompleteCallback(
	webrtc::EncodedImageCallback* callback)
{
	callback_ = callback;
	return 0;
}

int32_t AmfEncoder::Release()
{
	close();

	return 0;
}

int32_t AmfEncoder::Encode(const webrtc::VideoFrame& frame, const std::vector<webrtc::VideoFrameType>* frame_types)
{
#if USE_ENCODER_PERFORMANCE_TEST
	g_es->EncodeFrameStart();
#endif

	// new ========================================================================
	bool send_key_frame = false;
	if(frame_types!=nullptr && frame_types->size()>0) {
        send_key_frame = ((*frame_types)[0]==VideoFrameType::kVideoFrameKey);
		//if(send_key_frame)
		//	ELOGD << format_string("[AmfEncoder::Encode] requested KeyFrame - kVideoFrameKey(%d)", send_key_frame);
	}
    if(first_frame_) {
		ELOGD << format_string("[AmfEncoder::Encode] requested KeyFrame - first_frame_(%d)", first_frame_);
		first_frame_ = false;
        send_key_frame = true;
	}

	std::vector<std::vector<uint8_t>> frame_packet;
	EncodeFrame(frame, frame_packet, send_key_frame);

	// ============================================================================

	if (frame_packet.size() == 0) {
		ELOGD << format_string("[AmfEncoder::Encode] empty frame_packet time(%d)", frame.timestamp());
		goto final;
	}

	do
	{
		std::vector<uint8_t> &encoded_output_buffer_ = frame_packet[0];	
		bool isEncodedKeyFrame = VideoEncoderUtil::IsKeyFrame(frame_packet[0]);
		if(isEncodedKeyFrame) {
			InsertHeader(encoded_output_buffer_);
		}

		RTPFragmentationHeader fragment_header;
		if (!VideoEncoderUtil::MakeRtpFragmentationHeader(encoded_output_buffer_, fragment_header, isEncodedKeyFrame)) {
			ELOGD << format_string("[AmfEncoder::Encode] failed to MakeRtpFragmentationHeader(%d)", frame.timestamp());
			goto final;
		}

		EncodedImage encoded_image_;
		rtc::scoped_refptr<webrtc::EncodedImageBuffer> encoded_image_buffer = webrtc::EncodedImageBuffer::Create(static_cast<const uint8_t*>(encoded_output_buffer_.data()), encoded_output_buffer_.size());	
		encoded_image_.SetEncodedData(std::move(encoded_image_buffer));
		encoded_image_.set_size(encoded_output_buffer_.size());
        encoded_image_._encodedWidth = config_.width;
        encoded_image_._encodedHeight = config_.height;
        encoded_image_.SetTimestamp(frame.timestamp());
        encoded_image_.ntp_time_ms_ = frame.ntp_time_ms();
        encoded_image_.capture_time_ms_ = frame.render_time_ms();
        encoded_image_.rotation_ = frame.rotation();
		encoded_image_._frameType = (send_key_frame) ? VideoFrameType::kVideoFrameKey : VideoFrameType::kVideoFrameDelta;
        encoded_image_.timing_.flags = VideoSendTiming::kInvalid;
        encoded_image_.SetSpatialIndex(0);

        if (encoded_image_.size() > 0)
        {
            // Deliver encoded image.
            CodecSpecificInfo codec_specific;
			memset(&codec_specific, 0, sizeof(codec_specific));
            codec_specific.codecType = kVideoCodecH264;
            codec_specific.codecSpecific.H264.packetization_mode = H264PacketizationMode::NonInterleaved;
            callback_->OnEncodedImage(encoded_image_, &codec_specific, &fragment_header);
        }
	} while(false);

final:

#if USE_ENCODER_PERFORMANCE_TEST
	g_es->EncodeFrameEnd();
#endif

	return WEBRTC_VIDEO_CODEC_OK;
}

bool AmfEncoder::EncodeFrame(const VideoFrame& frame,
	std::vector<std::vector<uint8_t>>& frame_packet, bool is_key_frame)
{
	if (encoder==nullptr)
		return false;

	frame_packet.clear();

	struct encoder_frame  enc_frame;
	memset(&enc_frame, 0, sizeof(struct encoder_frame));

	//NV12
	webrtc::ConvertFromI420(frame, webrtc::VideoType::kNV12, 0, nv12_buff_.get());
	enc_frame.data[0] = nv12_y_plane_;
	enc_frame.data[1] = nv12_uv_plane_;
	enc_frame.linesize[0] = config_.width;
	enc_frame.linesize[1] = config_.width;
	
	bool received_packet = false;
	struct encoder_packet  enc_packet;
	memset(&enc_packet, 0, sizeof(struct encoder_packet));
	encoder->encode_frame(&enc_frame, &enc_packet, &received_packet, is_key_frame);

	if(received_packet)
	{
		frame_packet.push_back(std::vector<uint8_t>());
		frame_packet[0].insert(frame_packet[0].end(), &enc_packet.data[0], &enc_packet.data[enc_packet.size]);
	}

	return true;
}

void AmfEncoder::SetRates(const RateControlParameters& parameters)
{
	if (encoder==nullptr)
		return;

	bool is_update = config_.encoder_bitrate != parameters.bitrate.get_sum_bps();
	if(is_update)
	{
		ELOGD << format_string("[%s::SetRates] framerate(%lf) bitrate(%d Kbps) bandwidth(%ld Kbps) is_bw_limited(%d)", encoder_name_.c_str(), parameters.framerate_fps, parameters.bitrate.get_sum_kbps(), parameters.bandwidth_allocation.kbps(), parameters.bitrate.is_bw_limited());
		config_.encoder_bitrate = parameters.bitrate.get_sum_bps();
		encoder->update(config_);
	}
}

void AmfEncoder::OnPacketLossRateUpdate(float packet_loss_rate)
{
	if (packet_loss_rate>0)
		ELOGD << format_string("[%s::OnPacketLossRateUpdate] packet_loss_rate(%f)", encoder_name_.c_str(), packet_loss_rate);
}

void AmfEncoder::OnRttUpdate(int64_t rtt_ms)
{
	//ELOGD << format_string("[%s::OnRttUpdate] rtt_ms(%lld)", encoder_name_.c_str(), rtt_ms);
}

void AmfEncoder::OnLossNotification(const LossNotification& loss_notification)
{
	ELOGD << format_string("[%s::OnLossNotification] timestamp_of_last_decodable(%d) timestamp_of_last_received(%d)", encoder_name_.c_str(), loss_notification.timestamp_of_last_decodable, loss_notification.timestamp_of_last_received);
	ELOGD << format_string("[%s::OnLossNotification] dependencies_of_last_received_decodable(%d) last_received_decodable(%d)", encoder_name_.c_str(), loss_notification.dependencies_of_last_received_decodable, loss_notification.last_received_decodable);
}

VideoEncoder::EncoderInfo AmfEncoder::GetEncoderInfo() const
{
	EncoderInfo info = __super::GetEncoderInfo();
	info.implementation_name = encoder_name_;
	info.is_hardware_accelerated = true;
	info.supports_simulcast = false;
	return info;
}

bool AmfEncoder::IsKeyFrame(const std::vector<uint8_t>& encoded_output_buffer)
{
	RTPFragmentationHeader frag_header;

	auto encoded_buffer_size = encoded_output_buffer.size();
    std::vector<H264::NaluIndex> NALUidx;
    auto p_nal = encoded_output_buffer.data();
    NALUidx = H264::FindNaluIndices(p_nal, encoded_buffer_size);
	size_t i_nal = NALUidx.size();
    if (i_nal == 0)
    {
		return false;
    }

    if (i_nal == 1)
    {
        NALUidx[0].payload_size = encoded_buffer_size - NALUidx[0].payload_start_offset;
    }
    else for (size_t i = 0; i < i_nal; i++)
    {
        NALUidx[i].payload_size = i + 1 >= i_nal ? encoded_buffer_size - NALUidx[i].payload_start_offset : NALUidx[i + 1].start_offset - NALUidx[i].payload_start_offset;
    }

	for (size_t nal_index = 0; nal_index < i_nal; nal_index++)
	{
		uint8_t naluType = H264::ParseNaluType(p_nal[NALUidx[nal_index].payload_start_offset]);
		if (naluType == (uint8_t)AVC_NAL_TYPE::AVC_NAL_IDR) {
			return true;
		}
	}

	return false;
}

void AmfEncoder::InsertHeader(std::vector<uint8_t>& encoded_output_buffer)
{
	// insert sps, pps
	size_t size;
	uint8_t *header = NULL;
	encoder->get_extra_data(&header, &size);
	encoded_output_buffer.insert(encoded_output_buffer.begin(), &header[0], &header[size]);
}

bool AmfEncoder::open() 
{
	frame_length_ = config_.width * config_.height + ((config_.width + 1) >> 1) * ((config_.height + 1) >> 1) * 2;
	nv12_buff_.reset(new uint8_t[frame_length_]);
	nv12_y_plane_ = (uint8_t *)nv12_buff_.get();
	nv12_uv_plane_ = nv12_y_plane_ + config_.width * config_.height;

	try {
		Plugin::AMD::AMF::Initialize();
	} catch (std::exception e) {
		PLOG_ERROR("Encountered Exception during AMF initialization: %s", e.what());
		return false;
	} catch (...) {
		PLOG_ERROR("Unexpected Exception during AMF initialization.");
		return false;
	}

	// Initialize Graphics APIs
	Plugin::API::InitializeAPIs();

	try {
		encoder = new Plugin::Interface::H264Interface(config_);
		encoder->open(config_);
	} catch (std::exception e) {
		PLOG_ERROR("%s", e.what());
		return false;
	}	

	return true;
}

bool AmfEncoder::close() 
{
	if (encoder==nullptr)
		return true;

	encoder->close();
	delete encoder;
	encoder = nullptr;

	return true;
}

} // namespace webrtc
