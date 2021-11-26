#include "nv_encoder.h"

#include <limits>
#include <string>

#include "third_party/openh264/src/codec/api/svc/codec_api.h"
#include "third_party/openh264/src/codec/api/svc/codec_app_def.h"
#include "third_party/openh264/src/codec/api/svc/codec_def.h"
#include "third_party/openh264/src/codec/api/svc/codec_ver.h"
#include "absl/strings/match.h"
#include "common_video/h264/h264_common.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "modules/video_coding/utility/simulcast_rate_allocator.h"
#include "modules/video_coding/utility/simulcast_utility.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/time_utils.h"
#include "system_wrappers/include/metrics.h"
#include "third_party/libyuv/include/libyuv/convert.h"
#include "third_party/libyuv/include/libyuv/scale.h"
#include "easylogging++.h"
#include "../src/util.h"
#include "VideoEncoderUtil.h"
#include "EncoderStatistics.h"
#include "avc_util.hpp"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")

#define NO_SIMULCAST_M70

namespace webrtc {

namespace {

const bool kOpenH264EncoderDetailedLogging = false;

// QP scaling thresholds.
static const int kLowH264QpThreshold = 24;
static const int kHighH264QpThreshold = 37;

// Used by histograms. Values of entries should not be changed.
enum NvVideoEncoderEvent
{
	kH264EncoderEventInit = 0,
	kH264EncoderEventError = 1,
	kH264EncoderEventMax = 16,
};

int NumberOfThreads(int width, int height, int number_of_cores) 
{
	// TODO(hbos): In Chromium, multiple threads do not work with sandbox on Mac,
	// see crbug.com/583348. Until further investigated, only use one thread.
	//  if (width * height >= 1920 * 1080 && number_of_cores > 8) {
	//    return 8;  // 8 threads for 1080p on high perf machines.
	//  } else if (width * height > 1280 * 960 && number_of_cores >= 6) {
	//    return 3;  // 3 threads for 1080p.
	//  } else if (width * height > 640 * 480 && number_of_cores >= 3) {
	//    return 2;  // 2 threads for qHD/HD.
	//  } else {
	//    return 1;  // 1 thread for VGA or less.
	//  }
	// TODO(sprang): Also check sSliceArgument.uiSliceNum om GetEncoderPrams(),
	//               before enabling multithreading here.
  return 1;
}

VideoFrameType ConvertToVideoFrameType(EVideoFrameType type) 
{
	switch (type) 
	{
	case videoFrameTypeIDR:
		return VideoFrameType::kVideoFrameKey;
	case videoFrameTypeSkip:
	case videoFrameTypeI:
	case videoFrameTypeP:
	case videoFrameTypeIPMixed:
		return VideoFrameType::kVideoFrameDelta;
	case videoFrameTypeInvalid:
		break;
	}
	RTC_NOTREACHED() << "Unexpected/invalid frame type: " << type;
	return VideoFrameType::kEmptyFrame;
}

}  // namespace

// Helper method used by NvVideoEncoder::Encode.
// Copies the encoded bytes from |info| to |encoded_image| and updates the
// fragmentation information of |frag_header|. The |encoded_image->_buffer| may
// be deleted and reallocated if a bigger buffer is required.
//
// After OpenH264 encoding, the encoded bytes are stored in |info| spread out
// over a number of layers and "NAL units". Each NAL unit is a fragment starting
// with the four-byte start code {0,0,0,1}. All of this data (including the
// start codes) is copied to the |encoded_image->_buffer| and the |frag_header|
// is updated to point to each fragment, with offsets and lengths set as to
// exclude the start codes.

static void RtpFragmentize(EncodedImage* encoded_image,
                           const VideoFrameBuffer& frame_buffer,
                           std::vector<std::vector<uint8_t>>& frame_packet,
                           RTPFragmentationHeader* frag_header) 
{
#ifdef NO_SIMULCAST_M70
	/*
	size_t required_capacity = 0;
	encoded_image->_size = 0;
	for (auto packet : frame_packet) {
		required_capacity += packet.size();
	}

	if (encoded_image->_size < required_capacity) {
		size_t new_capacity = CalcBufferSize(VideoType::kI420, frame_buffer.width(),frame_buffer.height());
		if (new_capacity < required_capacity) {
			new_capacity = required_capacity;
		}
		encoded_image->_buffer = new uint8_t[new_capacity];
		encoded_image->_size = new_capacity;
	}

	size_t index = 0;
	for (auto packet : frame_packet) {
		memcpy(encoded_image->_buffer, packet.data(), packet.size());
		break;
	}

	encoded_image->_size = encoded_image->_size + index;
	// char data[1024] = {0};
	// sprintf(data, "Data: %02x %02x %02x %02x %02x %02x",
	//        frame_packet[0].data()[0], frame_packet[0].data()[1],
	//        frame_packet[0].data()[2], frame_packet[0].data()[3],
	//        frame_packet[0].data()[4], frame_packet[0].data()[5]);
	// RTC_LOG(LS_ERROR) << data;

	std::vector<webrtc::H264::NaluIndex> nalus = webrtc::H264::FindNaluIndices(
		encoded_image->_buffer, encoded_image->_size);
	size_t fragments_count = nalus.size();
	// const uint8_t start_code[4] = {0, 0, 0, 1};
	frag_header->VerifyAndAllocateFragmentationHeader(fragments_count);
	for (size_t i = 0; i < nalus.size(); i++) {
		frag_header->fragmentationOffset[i] = nalus[i].payload_start_offset;
		frag_header->fragmentationLength[i] = nalus[i].payload_size;
		frag_header->fragmentationPlType[i] = 0;
		frag_header->fragmentationTimeDiff[i] = 0;
	}
	*/
#else
	size_t required_capacity = 0;
	encoded_image->set_size(0);
	for (auto packet : frame_packet) {
		required_capacity += packet.size();
	}

	if (encoded_image->capacity() < required_capacity) {
		size_t new_capacity = CalcBufferSize(VideoType::kI420, frame_buffer.width(),frame_buffer.height());
		if (new_capacity < required_capacity) {
			new_capacity = required_capacity;
		}
		encoded_image->Allocate(new_capacity);
	}

	size_t index = 0;
	for (auto packet : frame_packet) {
		memcpy(encoded_image->data() + encoded_image->size() + index, packet.data(), packet.size());
		index += packet.size();
	}

	encoded_image->set_size(encoded_image->size() + index);
	// char data[1024] = {0};
	// sprintf(data, "Data: %02x %02x %02x %02x %02x %02x",
	//        frame_packet[0].data()[0], frame_packet[0].data()[1],
	//        frame_packet[0].data()[2], frame_packet[0].data()[3],
	//        frame_packet[0].data()[4], frame_packet[0].data()[5]);
	// RTC_LOG(LS_ERROR) << data;

	std::vector<webrtc::H264::NaluIndex> nalus = webrtc::H264::FindNaluIndices(
		encoded_image->data(), encoded_image->size());
	size_t fragments_count = nalus.size();
	// const uint8_t start_code[4] = {0, 0, 0, 1};
	frag_header->VerifyAndAllocateFragmentationHeader(fragments_count);
	for (size_t i = 0; i < nalus.size(); i++) {
		frag_header->fragmentationOffset[i] = nalus[i].payload_start_offset;
		frag_header->fragmentationLength[i] = nalus[i].payload_size;
		frag_header->fragmentationPlType[i] = 0;
		frag_header->fragmentationTimeDiff[i] = 0;
	}
#endif//NO_SIMULCAST_M70
}

NvEncoder::NvEncoder(const cricket::VideoCodec& codec, int framerate)
    : packetization_mode_(H264PacketizationMode::SingleNalUnit),
	  m_framerate(framerate),
      max_payload_size_(0),
      number_of_cores_(0),
      encoded_image_callback_(nullptr),
      has_reported_init_(false),
      has_reported_error_(false),
      num_temporal_layers_(1),
      tl0sync_limit_(0) 
{
	encoder_name_ = "NvEncoder";

	//RTC_CHECK(absl::EqualsIgnoreCase(codec.name, cricket::kH264CodecName));
	RTC_CHECK(strcmp(codec.name.c_str(), cricket::kH264CodecName)==0);
	std::string packetization_mode_string;
	if (codec.GetParam(cricket::kH264FmtpPacketizationMode, &packetization_mode_string) 
		&& packetization_mode_string == "1") {
		packetization_mode_ = H264PacketizationMode::NonInterleaved;
	}

	encoded_images_.reserve(kMaxSimulcastStreams);
	nv_encoders_.reserve(kMaxSimulcastStreams);
	configurations_.reserve(kMaxSimulcastStreams);
	image_buffer_ = nullptr;
}

NvEncoder::~NvEncoder() 
{
	Release();
}

int32_t NvEncoder::InitEncode(const VideoCodec* inst,
                                   int32_t number_of_cores,
                                   size_t max_payload_size) 
{
	ELOGD << format_string("[NvEncoder::InitEncode]");

	m_bitrate = 0;

	ReportInit();
	if (!inst || inst->codecType != kVideoCodecH264) {
		ReportError();
		return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
	}
	if (inst->maxFramerate == 0) {
		ReportError();
		return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
	}
	if (inst->width < 1 || inst->height < 1) {
		ReportError();
		return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
	}

	int32_t release_ret = Release();
	if (release_ret != WEBRTC_VIDEO_CODEC_OK) {
		ReportError();
		return release_ret;
	}

	int number_of_streams = SimulcastUtility::NumberOfSimulcastStreams(*inst);
	bool doing_simulcast = (number_of_streams > 1);

#ifdef NO_SIMULCAST_M70

#else
	if (doing_simulcast && !SimulcastUtility::ValidSimulcastParameters(*inst, number_of_streams)) {
		return WEBRTC_VIDEO_CODEC_ERR_SIMULCAST_PARAMETERS_NOT_SUPPORTED;
	}
#endif//NO_SIMULCAST_M70

	assert(number_of_streams == 1);

	encoded_images_.resize(number_of_streams);
	nv_encoders_.resize(number_of_streams);
	configurations_.resize(number_of_streams);

	number_of_cores_ = number_of_cores;
	max_payload_size_ = max_payload_size;
	codec_ = *inst;

	// Code expects simulcastStream resolutions to be correct, make sure they are
	// filled even when there are no simulcast layers.
	if (codec_.numberOfSimulcastStreams == 0) {
		codec_.simulcastStream[0].width = codec_.width;
		codec_.simulcastStream[0].height = codec_.height;
	}

#ifdef NO_SIMULCAST_M70
	num_temporal_layers_ = 0;
#else
	num_temporal_layers_ = codec_.H264()->numberOfTemporalLayers;
#endif//NO_SIMULCAST_M70

	for (int i = 0, idx = number_of_streams - 1; i < number_of_streams;  ++i, --idx) {
		// Store nvidia encoder.
		nv_encoders_[i] = nvenc_info.create();

		// Set internal settings from codec_settings
		configurations_[i].simulcast_idx = idx;
		configurations_[i].sending = false;
		configurations_[i].width = codec_.simulcastStream[idx].width;
		configurations_[i].height = codec_.simulcastStream[idx].height;
		configurations_[i].max_frame_rate = static_cast<float>(codec_.maxFramerate);
		configurations_[i].frame_dropping_on = codec_.H264()->frameDroppingOn;
		configurations_[i].key_frame_interval = codec_.H264()->keyFrameInterval;

		// Codec_settings uses kbits/second; encoder uses bits/second.
		configurations_[i].max_bps = codec_.maxBitrate * 1000;
		configurations_[i].target_bps = codec_.startBitrate * 1000;
	
		encoder_config nvenc_config;
		nvenc_config.codec = "h264";
		nvenc_config.format = DXGI_FORMAT_B8G8R8A8_UNORM;
		nvenc_config.width = configurations_[i].width;
		nvenc_config.height = configurations_[i].height;
		nvenc_config.framerate = (uint32_t)configurations_[i].max_frame_rate;
		nvenc_config.gop = configurations_[i].key_frame_interval;
		nvenc_config.bitrate = configurations_[i].target_bps;

		switch (g_cec->quality_preset)
		{
			case EncoderQualityPreset::Speed:
				nvenc_config.quality_preset = NvencQualityPreset::Speed;
				break;	
			case EncoderQualityPreset::Balanced:
				nvenc_config.quality_preset = NvencQualityPreset::Balanced;
				break;	
			case EncoderQualityPreset::Quality:
				nvenc_config.quality_preset = NvencQualityPreset::Quality;
				break;
			default:
				nvenc_config.quality_preset = NvencQualityPreset::Speed;
				break;	
		}

		switch (g_cec->rate_control)
		{
			case EncoderRateControl::VBR:
				nvenc_config.rate_control = NvencRateControl::VBR;
				break;	
			case EncoderRateControl::CBR:
				nvenc_config.rate_control = NvencRateControl::CBR;
				break;
			default:
				nvenc_config.rate_control = NvencRateControl::VBR; 
		}

		ELOGD << format_string("[NvEncoder::InitEncode] video(%d %d)", nvenc_config.width, nvenc_config.height);
		ELOGD << format_string("[NvEncoder::InitEncode] fps_num(%d) max_bitrate(%d) target_bitrate(%d)", nvenc_config.framerate, nvenc_config.bitrate, nvenc_config.bitrate);
		ELOGD << format_string("[NvEncoder::InitEncode] quality_preset(%d) common_rate_control(%d), rate_control(%d) ", nvenc_config.quality_preset, g_cec->rate_control, nvenc_config.rate_control);

		if (!nvenc_info.init(nv_encoders_[i], &nvenc_config)) {
			Release();
			ReportError();
			return WEBRTC_VIDEO_CODEC_ERROR;
		}
		
		image_buffer_.reset(new uint8_t[configurations_[i].width * configurations_[i].height * 10]);

		// TODO(pbos): Base init params on these values before submitting.
		video_format_ = EVideoFormatType::videoFormatI420;

		// Initialize encoded image. Default buffer size: size of unencoded data.
		const size_t new_capacity = CalcBufferSize(VideoType::kI420, 
			codec_.simulcastStream[idx].width, codec_.simulcastStream[idx].height);
#ifdef NO_SIMULCAST_M70
		
#else
		encoded_images_[i].Allocate(new_capacity);
		encoded_images_[i]._completeFrame = true;
		encoded_images_[i]._encodedWidth = codec_.simulcastStream[idx].width;
		encoded_images_[i]._encodedHeight = codec_.simulcastStream[idx].height;
		encoded_images_[i].set_size(0);
#endif//NO_SIMULCAST_M70
	}

	g_es->InitEncode(encoder_name_);

	SimulcastRateAllocator init_allocator(codec_);
	VideoBitrateAllocation allocation = init_allocator.Allocate(VideoBitrateAllocationParameters(DataRate::KilobitsPerSec(codec_.startBitrate), codec_.maxFramerate));
	SetRates(RateControlParameters(allocation, codec_.maxFramerate));

	ELOGD << format_string("[NvEncoder::InitEncode] finish");

	return WEBRTC_VIDEO_CODEC_OK;
}

int32_t NvEncoder::Release() 
{
	ELOGD << format_string("[NvEncoder::Release]");

	while (!nv_encoders_.empty()) 
	{
		void* nv_encoder = nv_encoders_.back();
		if (nv_encoder) {
			ELOGD << format_string("[NvEncoder::Release] nvenc_info.destroy()");
			nvenc_info.destroy(&nv_encoder);
			nv_encoder = nullptr;
		}
		nv_encoders_.pop_back();
	}

	configurations_.clear();
	encoded_images_.clear();

	return WEBRTC_VIDEO_CODEC_OK;
}

int32_t NvEncoder::RegisterEncodeCompleteCallback(EncodedImageCallback* callback) 
{
	encoded_image_callback_ = callback;
	return WEBRTC_VIDEO_CODEC_OK;
}

int32_t NvEncoder::Encode(const webrtc::VideoFrame& frame, const std::vector<webrtc::VideoFrameType>* frame_types)
{
#if USE_ENCODER_PERFORMANCE_TEST
	g_es->EncodeFrameStart();
#endif

	if (nv_encoders_.empty()) {
		ReportError();
		return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
	}
	if (!encoded_image_callback_) {
		RTC_LOG(LS_WARNING)
			<< "InitEncode() has been called, but a callback function "
			<< "has not been set with RegisterEncodeCompleteCallback()";
		ReportError();
		return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
	}

	rtc::scoped_refptr<const I420BufferInterface> frame_buffer = frame.video_frame_buffer()->ToI420();

	bool send_key_frame = false;
	for (size_t i = 0; i < configurations_.size(); ++i) {
		if (configurations_[i].key_frame_request && configurations_[i].sending) {
			send_key_frame = true;
			//ELOGD << format_string("[NvEncoder::Encode] requested KeyFrame - index(%d) send_key_frame(%d) key_frame_request(%d)", i, send_key_frame, configurations_[i].key_frame_request);
			break;
		}
	}
	if (!send_key_frame && frame_types) {
		for (size_t i = 0; i < frame_types->size() && i < configurations_.size(); ++i) {
			if ((*frame_types)[i] == VideoFrameType::kVideoFrameKey && configurations_[i].sending) {
				send_key_frame = true;
				//ELOGD << format_string("[NvEncoder::Encode] requested KeyFrame - index(%d) send_key_frame(%d) kVideoFrameKey(%d)", i, send_key_frame, (*frame_types)[i] == VideoFrameType::kVideoFrameKey);
				break;
			}
		}
	}

	RTC_DCHECK_EQ(configurations_[0].width, frame_buffer->width());
	RTC_DCHECK_EQ(configurations_[0].height, frame_buffer->height());

	// Encode image for each layer.
	for (size_t i = 0; i < nv_encoders_.size(); ++i) {
		if (!configurations_[i].sending) {
			continue;
		}

		if (frame_types != nullptr) {
			// Skip frame?
			if ((*frame_types)[i] == VideoFrameType::kEmptyFrame) {
				continue;
			}
		}

		if (send_key_frame) {			
			if (!nv_encoders_.empty() && nv_encoders_[i]) {
				nvenc_info.request_idr(nv_encoders_[i]);
			}

			configurations_[i].key_frame_request = false;
		}

		// EncodeFrame output.
		SFrameBSInfo info;
		memset(&info, 0, sizeof(SFrameBSInfo));
		std::vector<std::vector<uint8_t>> frame_packet;
		EncodeFrame((int)i, frame, frame_packet);

#ifdef NO_SIMULCAST_M70		
		if (frame_packet.size() == 0) {
			ELOGD << format_string("[NvEncoder::Encode] empty frame_packet time(%d)", frame.timestamp());
			goto final;
		}

		do
		{
			std::vector<uint8_t> &encoded_output_buffer_ = frame_packet[0];	
			RTPFragmentationHeader fragment_header;
			if (!VideoEncoderUtil::MakeRtpFragmentationHeader(encoded_output_buffer_, fragment_header, send_key_frame)) {
				ELOGD << format_string("[NvEncoder::Encode] failed to MakeRtpFragmentationHeader(%d)", frame.timestamp());
				goto final;
			}

			EncodedImage encoded_image_;
			rtc::scoped_refptr<webrtc::EncodedImageBuffer> encoded_image_buffer = webrtc::EncodedImageBuffer::Create(static_cast<const uint8_t*>(encoded_output_buffer_.data()), encoded_output_buffer_.size());
			encoded_image_.SetEncodedData(std::move(encoded_image_buffer));
			encoded_image_.set_size(encoded_output_buffer_.size());
			encoded_image_._encodedWidth = configurations_[0].width;
			encoded_image_._encodedHeight = configurations_[0].height;
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
				encoded_image_callback_->OnEncodedImage(encoded_image_, &codec_specific, &fragment_header);
			}
		} while(false);
#else
		if (frame_packet.size() == 0) {
			return WEBRTC_VIDEO_CODEC_OK;
		}
		else {
			if (frame_packet[0].data()[4] == 0x67) {
				info.eFrameType = videoFrameTypeIDR;
			}
			else if (frame_packet[0].data()[4] == 0x61) {
				info.eFrameType = videoFrameTypeP;
			}
			else {
				return WEBRTC_VIDEO_CODEC_OK;
			}
		}

		encoded_images_[i]._encodedWidth = configurations_[i].width;
		encoded_images_[i]._encodedHeight = configurations_[i].height;
		encoded_images_[i].SetTimestamp(input_frame.timestamp());
		encoded_images_[i].ntp_time_ms_ = input_frame.ntp_time_ms();
		encoded_images_[i].capture_time_ms_ = input_frame.render_time_ms();
		encoded_images_[i].rotation_ = input_frame.rotation();
//		encoded_images_[i].SetColorSpace(input_frame.color_space());
		encoded_images_[i].content_type_ =
			(codec_.mode == VideoCodecMode::kScreensharing)
				? VideoContentType::SCREENSHARE
				: VideoContentType::UNSPECIFIED;
		encoded_images_[i].timing_.flags = VideoSendTiming::kInvalid;
		encoded_images_[i]._frameType = ConvertToVideoFrameType(info.eFrameType);
		encoded_images_[i].SetSpatialIndex(configurations_[i].simulcast_idx);

		// Split encoded image up into fragments. This also updates
		// |encoded_image_|.
		RTPFragmentationHeader frag_header;
		RtpFragmentize(&encoded_images_[i], *frame_buffer, frame_packet, &frag_header);

		// Encoder can skip frames to save bandwidth in which case
		// |encoded_images_[i]._length| == 0.
		if (encoded_images_[i]._size > 0) {
			// Parse QP.
			//h264_bitstream_parser_.ParseBitstream(encoded_images_[i].data(),
			//                                      encoded_images_[i].size());
			//h264_bitstream_parser_.GetLastSliceQp(&encoded_images_[i].qp_);
			encoded_images_[i].qp_ = 10;

			// Deliver encoded image.
			CodecSpecificInfo codec_specific;
			codec_specific.codecType = kVideoCodecH264;
			codec_specific.codecSpecific.H264.packetization_mode = packetization_mode_;
			codec_specific.codecSpecific.H264.temporal_idx = kNoTemporalIdx;
			codec_specific.codecSpecific.H264.idr_frame = info.eFrameType == videoFrameTypeIDR;
			codec_specific.codecSpecific.H264.base_layer_sync = false;

			// if (info.eFrameType == videoFrameTypeIDR &&
			//    encoded_images_[i]._frameType == kVideoFrameKey) {
			//  RTC_LOG(LS_ERROR) << "send idr frame - " << encoded_images_[i].size();
			//}

			encoded_image_callback_->OnEncodedImage(encoded_images_[i], &codec_specific, &frag_header);
		}
#endif//NO_SIMULCAST_M70
	}

final:

#if USE_ENCODER_PERFORMANCE_TEST
	g_es->EncodeFrameEnd();
#endif

	return WEBRTC_VIDEO_CODEC_OK;
}

void NvEncoder::SetRates(const RateControlParameters& parameters)
{
	if (nv_encoders_.empty()) {
		RTC_LOG(LS_WARNING) << "SetRates() while uninitialized.";
		return;
	}

	if (parameters.framerate_fps < 1.0) {
		RTC_LOG(LS_WARNING) << "Invalid frame rate: " << parameters.framerate_fps;
		return;
	}

	if (parameters.bitrate.get_sum_bps() == 0) {
		// Encoder paused, turn off all encoding.
		for (size_t i = 0; i < configurations_.size(); ++i) {
			configurations_[i].SetStreamState(false);
		}
		return;
	}

	bool is_update = (m_bitrate != parameters.bitrate.get_sum_bps());
	if (!is_update)
		return;

	m_bitrate = parameters.bitrate.get_sum_bps();
	
	ELOGD << format_string("[%s::SetRates] framerate(%lf) bitrate(%d Kbps) bandwidth(%ld Kbps) is_bw_limited(%d)", encoder_name_.c_str(), parameters.framerate_fps, parameters.bitrate.get_sum_kbps(), parameters.bandwidth_allocation.kbps(), parameters.bitrate.is_bw_limited());

	codec_.maxFramerate = static_cast<uint32_t>(parameters.framerate_fps);

	size_t stream_idx = nv_encoders_.size() - 1;
	for (size_t i = 0; i < nv_encoders_.size(); ++i, --stream_idx) {
		// Update layer config.
		configurations_[i].target_bps = parameters.bitrate.GetSpatialLayerSum(stream_idx);
		configurations_[i].max_frame_rate = (float)m_framerate;

		if (configurations_[i].target_bps) {
			configurations_[i].SetStreamState(true);

			if (nv_encoders_[i]) {
				nvenc_info.set_bitrate(nv_encoders_[i], configurations_[i].target_bps);
				nvenc_info.set_framerate(nv_encoders_[i], (uint32_t)configurations_[i].max_frame_rate);
			}
			else {
				configurations_[i].SetStreamState(false);
			}

		} else {
			configurations_[i].SetStreamState(false);
		}
	}
}

void NvEncoder::OnPacketLossRateUpdate(float packet_loss_rate)
{	
	if (packet_loss_rate>0)
		ELOGD << format_string("[%s::OnPacketLossRateUpdate] packet_loss_rate(%f)", encoder_name_.c_str(), packet_loss_rate);
}

void NvEncoder::OnRttUpdate(int64_t rtt_ms)
{
	//ELOGD << format_string("[%s::OnRttUpdate] rtt_ms(%lld)", encoder_name_.c_str(), rtt_ms);
}

void NvEncoder::OnLossNotification(const LossNotification& loss_notification)
{
	ELOGD << format_string("[%s::OnLossNotification] timestamp_of_last_decodable(%d) timestamp_of_last_received(%d)", encoder_name_.c_str(), loss_notification.timestamp_of_last_decodable, loss_notification.timestamp_of_last_received);
	ELOGD << format_string("[%s::OnLossNotification] dependencies_of_last_received_decodable(%d) last_received_decodable(%d)", encoder_name_.c_str(), loss_notification.dependencies_of_last_received_decodable, loss_notification.last_received_decodable);
}

VideoEncoder::EncoderInfo NvEncoder::GetEncoderInfo() const
{
	EncoderInfo info = __super::GetEncoderInfo();
	info.implementation_name = encoder_name_;
	info.is_hardware_accelerated = true;
	info.supports_simulcast = false;
	return info;
}

void NvEncoder::ReportInit() 
{
	if (has_reported_init_)
		return;
	RTC_HISTOGRAM_ENUMERATION("WebRTC.Video.NvEncoder.Event",
							kH264EncoderEventInit, kH264EncoderEventMax);
	has_reported_init_ = true;
}

void NvEncoder::ReportError() 
{
	if (has_reported_error_)
		return;
	RTC_HISTOGRAM_ENUMERATION("WebRTC.Video.NvEncoder.Event",
							kH264EncoderEventError, kH264EncoderEventMax);
	has_reported_error_ = true;
}
/*
VideoEncoder::EncoderInfo NvEncoder::GetEncoderInfo() const 
{
	EncoderInfo info;
	info.supports_native_handle = false;
	info.implementation_name = "NvEncoder";
	info.scaling_settings = VideoEncoder::ScalingSettings(kLowH264QpThreshold, kHighH264QpThreshold);
	info.is_hardware_accelerated = true;
	info.has_internal_source = false;
	return info;
}
*/
void NvEncoder::LayerConfig::SetStreamState(bool send_stream) 
{
	if (send_stream && !sending) {
		// Need a key frame if we have not sent this stream before.
		key_frame_request = true;
	}
	sending = send_stream;
}

bool NvEncoder::EncodeFrame(int index, const VideoFrame& input_frame,
    std::vector<std::vector<uint8_t>>& frame_packet) 
{
	frame_packet.clear();
	if (nv_encoders_.empty() || !nv_encoders_[index]) {
		return false;
	}

	if (video_format_ == EVideoFormatType::videoFormatI420) {
		if (image_buffer_ != nullptr) {
			if (webrtc::ConvertFromI420(input_frame, webrtc::VideoType::kARGB, 0,
										image_buffer_.get()) < 0) {
				return false;
			}
		} 
		else {
			return false;
		}
	}

	int width = input_frame.width();
	int height = input_frame.height();
	ID3D11Texture2D* texture = nvenc_info.get_texture(nv_encoders_[index]);
	ID3D11DeviceContext* context = nvenc_info.get_context(nv_encoders_[index]);

	if (texture && context) {
		D3D11_MAPPED_SUBRESOURCE dsec = {0};
		HRESULT hr = context->Map(texture, D3D11CalcSubresource(0, 0, 0), D3D11_MAP_WRITE, 0, &dsec);
		if (SUCCEEDED(hr)) {
			// nv12
#ifdef NO_SIMULCAST_M70
			for (int y = 0; y < height; y++) {
				memcpy((uint8_t*)dsec.pData + y * dsec.RowPitch,
						image_buffer_.get() + y * width * 4, width * 4);
			}
#else
			uint8_t* y_plane_ = image_buffer_.get();
			for (int i = 0; i < height; i++) {
				memcpy((uint8_t*)dsec.pData + dsec.RowPitch * i, y_plane_ + width * i, width);
			}
			uint8_t* uv_plane_ = image_buffer_.get() + width * height;
			for (int i = 0; i < height / 2; i++) {
				memcpy((uint8_t*)dsec.pData + dsec.RowPitch * (height + i), uv_plane_ + width * i, width);
			}
#endif//NO_SIMULCAST_M70

			context->Unmap(texture, D3D11CalcSubresource(0, 0, 0));
			nvenc_info.encode_texture(nv_encoders_[index], texture, frame_packet);
		}
	}

	return true;
}

}  // namespace webrtc
