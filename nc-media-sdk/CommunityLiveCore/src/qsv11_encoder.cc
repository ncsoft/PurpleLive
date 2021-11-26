#include "qsv11_encoder.h"

#include "rtc_base/time_utils.h"
#include "rtc_base/logging.h"
#include "media/base/h264_profile_level_id.h"
#include "modules/video_coding/include/video_codec_interface.h"
#include "modules/include/module_common_types.h"
#include "common_video/h264/h264_common.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "rtc_base/checks.h"

#include <sys/timeb.h>

#include "qsv/QSV_Encoder.h"
#include "easylogging++.h"
#include "../src/util.h"
#include "VideoEncoderUtil.h"
#include "EncoderStatistics.h"
#include "avc_util.hpp"

#define SIMPLE_ENCODER_X264_LOWCPU             "x264_lowcpu"
#define SIMPLE_ENCODER_QSV                     "qsv"
#define SIMPLE_ENCODER_NVENC                   "nvenc"
#define SIMPLE_ENCODER_AMD                     "amd"

namespace webrtc
{

QsvEncoder::QsvEncoder(const cricket::VideoCodec& codec, int framerate)
	: framerate_(framerate)
{
	encoder_name_ = "QsvEncoder";
}

QsvEncoder::~QsvEncoder() {

}

//WebRTC interfaces.
int32_t QsvEncoder::InitEncode(const webrtc::VideoCodec* codec_settings,
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
				config_.quality_preset = QsvQualityPreset::Speed; 
				break;	
			case EncoderQualityPreset::Balanced:
				config_.quality_preset = QsvQualityPreset::Balanced; 
				break;	
			case EncoderQualityPreset::Quality:
				config_.quality_preset = QsvQualityPreset::Quality; 
				break;	
			default:
				config_.quality_preset = QsvQualityPreset::Speed; 
		}

		switch (g_cec->rate_control)
		{
			case EncoderRateControl::VBR:
				config_.rate_control = QsvRateControl::AVBR; 
				break;	
			case EncoderRateControl::CBR:
				config_.rate_control = QsvRateControl::CBR; 
				break;
			default:
				config_.rate_control = QsvRateControl::AVBR; 
		}

		ELOGD << format_string("[QsvEncoder::InitEncode] video(%d %d)", config_.width, config_.height);
		ELOGD << format_string("[QsvEncoder::InitEncode] fps_num(%d) max_bitrate(%d) target_bitrate(%d)", config_.fps_num, config_.max_bitrate, config_.target_bitrate);
		ELOGD << format_string("[QsvEncoder::InitEncode] quality_preset(%d) common_rate_control(%d), rate_control(%d) ", config_.quality_preset, g_cec->rate_control, config_.rate_control);

		if (!open()) 
		{
			ret = -1;
			break;
		}
	} while (0);

	g_es->InitEncode(encoder_name_);

	return ret;
}

int32_t QsvEncoder::RegisterEncodeCompleteCallback(
	webrtc::EncodedImageCallback* callback)
{
	callback_ = callback;
	return 0;
}

int32_t QsvEncoder::Release()
{
	close();

	return 0;
}

int32_t QsvEncoder::Encode(const webrtc::VideoFrame& frame, const std::vector<webrtc::VideoFrameType>* frame_types)
{
	//RTC_LOG(LS_ERROR) << "Encode";

#if USE_ENCODER_PERFORMANCE_TEST
	g_es->EncodeFrameStart();
#endif

	// new ========================================================================
	bool send_key_frame = false;
	if(frame_types!=nullptr && frame_types->size()>0) {
        send_key_frame = ((*frame_types)[0]==VideoFrameType::kVideoFrameKey);
		//if(send_key_frame)
		//	ELOGD << format_string("[QsvEncoder::Encode] requested KeyFrame - kVideoFrameKey(%d)", send_key_frame);
	}
    if(first_frame_) {
		ELOGD << format_string("[QsvEncoder::Encode] requested KeyFrame - first_frame_(%d)", first_frame_);
		first_frame_ = false;
        send_key_frame = true;
	}

	std::vector<std::vector<uint8_t>> frame_packet;
	EncodeFrame(frame, frame_packet, send_key_frame);

	// ============================================================================

	if (frame_packet.size() == 0) {
		ELOGD << format_string("[QsvEncoder::Encode] empty frame_packet time(%d)", frame.timestamp());
		goto final;
	}

	do
	{
		std::vector<uint8_t> &encoded_output_buffer_ = frame_packet[0];	
		RTPFragmentationHeader fragment_header;
		if (!VideoEncoderUtil::MakeRtpFragmentationHeader(encoded_output_buffer_, fragment_header, send_key_frame)) {
			ELOGD << format_string("[QsvEncoder::Encode] failed to MakeRtpFragmentationHeader(%d)", frame.timestamp());
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

void QsvEncoder::SetRates(const RateControlParameters& parameters)
{
	if(m_qsv==nullptr)
		return;	

	bool is_update = params_.nMaxBitRate != parameters.bitrate.get_sum_kbps();
	if(is_update)
	{
		ELOGD << format_string("[%s::SetRates] framerate(%lf) bitrate(%d Kbps) bandwidth(%ld Kbps) is_bw_limited(%d)", encoder_name_.c_str(), parameters.framerate_fps, parameters.bitrate.get_sum_kbps(), parameters.bandwidth_allocation.kbps(), parameters.bitrate.is_bw_limited());
		params_.nMaxBitRate = parameters.bitrate.get_sum_kbps();
		params_.nTargetBitRate = parameters.bitrate.get_sum_kbps();
		qsv_encoder_reconfig(m_qsv, &params_);
	}
}

void QsvEncoder::OnPacketLossRateUpdate(float packet_loss_rate)
{
	if (packet_loss_rate>0)
		ELOGD << format_string("[%s::OnPacketLossRateUpdate] packet_loss_rate(%f)", encoder_name_.c_str(), packet_loss_rate);
}

void QsvEncoder::OnRttUpdate(int64_t rtt_ms)
{
	//ELOGD << format_string("[%s::OnRttUpdate] rtt_ms(%lld)", encoder_name_.c_str(), rtt_ms);
}

void QsvEncoder::OnLossNotification(const LossNotification& loss_notification)
{
	ELOGD << format_string("[%s::OnLossNotification] timestamp_of_last_decodable(%d) timestamp_of_last_received(%d)", encoder_name_.c_str(), loss_notification.timestamp_of_last_decodable, loss_notification.timestamp_of_last_received);
	ELOGD << format_string("[%s::OnLossNotification] dependencies_of_last_received_decodable(%d) last_received_decodable(%d)", encoder_name_.c_str(), loss_notification.dependencies_of_last_received_decodable, loss_notification.last_received_decodable);
}

VideoEncoder::EncoderInfo QsvEncoder::GetEncoderInfo() const
{
	EncoderInfo info = __super::GetEncoderInfo();
	info.implementation_name = encoder_name_;
	info.is_hardware_accelerated = true;
	info.supports_simulcast = false;
	return info;
}

bool QsvEncoder::EncodeFrame(const VideoFrame& frame,
	std::vector<std::vector<uint8_t>>& frame_packet, bool is_key_frame)
{
	frame_packet.clear();

	struct qsv_encoder_frame  enc_frame;
	memset(&enc_frame, 0, sizeof(struct qsv_encoder_frame));

	//Check if encoder supports I420 input, otherwise, need to convert from I420 to NV12.
	/*
	{ //i420
		enc_frame.data[0] = (uint8_t *)frame.video_frame_buffer()->GetI420()->DataY();
		enc_frame.data[1] = (uint8_t *)frame.video_frame_buffer()->GetI420()->DataU();
		enc_frame.data[2] = (uint8_t *)frame.video_frame_buffer()->GetI420()->DataV();

		enc_frame.linesize[0] = frame.video_frame_buffer()->GetI420()->StrideY();
		enc_frame.linesize[1] = frame.video_frame_buffer()->GetI420()->StrideU();
		enc_frame.linesize[2] = frame.video_frame_buffer()->GetI420()->StrideV();
	}
	*/

	//NV12
	webrtc::ConvertFromI420(frame, webrtc::VideoType::kNV12, 0, nv12_buff_.get());
	enc_frame.data[0] = nv12_y_plane_;
	enc_frame.data[1] = nv12_uv_plane_;
	enc_frame.linesize[0] = config_.width;
	enc_frame.linesize[1] = config_.width;

	mfxBitstream* pBS = nullptr;
	mfxU64 qsvPTS = 0;// input_frame->pts * 90000 / fps_num;

//	if (enc_frame)
	int ret = qsv_encoder_encode(m_qsv, qsvPTS,
		enc_frame.data[0], enc_frame.data[1],
		enc_frame.linesize[0], enc_frame.linesize[1],
		&pBS, is_key_frame);
//	frame_packetwidth_
//	else
//		ret = qsv_encoder_encode(m_qsv, qsvPTS, NULL, NULL, 0, 0, &pBS);

	// copy frame data
	if (pBS == nullptr)
		return true;
	
	frame_packet.push_back(std::vector<uint8_t>());
	frame_packet[0].insert(frame_packet[0].end(), &pBS->Data[0], &pBS->Data[pBS->DataLength]);

	return true;
}

void load_headers(qsv_t* qsv)
{
	uint8_t *pSPS, *pPPS;
	uint16_t nSPS, nPPS;

	qsv_encoder_headers(qsv, &pSPS, &pPPS, &nSPS, &nPPS);
}

bool QsvEncoder::open() 
{
	frame_length_ = config_.width * config_.height + ((config_.width + 1) >> 1) * ((config_.height + 1) >> 1) * 2;
	nv12_buff_.reset(new uint8_t[frame_length_]);
	nv12_y_plane_ = (uint8_t *)nv12_buff_.get();
	nv12_uv_plane_ = nv12_y_plane_ + config_.width * config_.height;

	// setting params
	qsv_param_t &params = params_;
	switch (config_.quality_preset)
	{
		case QsvQualityPreset::Speed :
			params.nTargetUsage = MFX_TARGETUSAGE_BEST_SPEED;
			break;
		case QsvQualityPreset::Balanced :
			params.nTargetUsage = MFX_TARGETUSAGE_BALANCED;
			break;
		case QsvQualityPreset::Quality :
			params.nTargetUsage = MFX_TARGETUSAGE_BEST_QUALITY;
			break;
		default :
			params.nTargetUsage = MFX_TARGETUSAGE_BEST_SPEED;
	}

	params.nWidth		= config_.width;
	params.nHeight		= config_.height;
	//params.nAsyncDepth	= 4;
	params.nAsyncDepth	= 1;	// for remove delay from input to output
	params.nFpsNum		= config_.fps_num;	
	params.nFpsDen		= config_.fps_den;
	params.nTargetBitRate	= config_.target_bitrate/1000;
	params.nMaxBitRate		= config_.max_bitrate/1000;
	params.nCodecProfile	= MFX_PROFILE_AVC_HIGH;

	switch (config_.rate_control)
	{
		case QsvRateControl::AVBR : 
			params.nRateControl = MFX_RATECONTROL_AVBR;
			break;
		case QsvRateControl::CBR : 
			params.nRateControl = MFX_RATECONTROL_CBR;
			break;
		default :
			params.nRateControl	= MFX_RATECONTROL_AVBR;
	}

	params.nAccuracy		= 1000;
	params.nConvergence		= 1;
	params.nQPI				= 32;
	params.nQPP				= 28;
	params.nQPB				= 31;
	params.nLADEPTH			= 15;
	params.nKeyIntSec		= INFINITE / params.nFpsNum * params.nFpsDen;
	params.nbFrames			= 0;
	params.nICQQuality		= 23;
	params.bMBBRC			= true;

	m_qsv = qsv_encoder_open(&params);

	load_headers(m_qsv);

	unsigned short verMajor;
	unsigned short verMinor;
	qsv_encoder_version(&verMajor, &verMinor);

	return true;
}

bool QsvEncoder::close() 
{
	if (m_qsv)
	{
		qsv_encoder_close(m_qsv);
		m_qsv = nullptr;
	}

	return true;
}

} // namespace webrtc
