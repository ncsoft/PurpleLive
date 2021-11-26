#include "VideoCapturer.h"
#include "api/video/i420_buffer.h"
#include <libyuv.h>

VideoCapturer::VideoCapturer() {}

VideoCapturer::~VideoCapturer() = default;

void VideoCapturer::OnFrameCaptured(uint8_t* videoFrame, const webrtc::VideoCaptureCapability& frameInfo, int64_t captureTime)
{
	const int32_t width = frameInfo.width;
	const int32_t height = frameInfo.height;

	// Calculate size
	uint32_t size;
	if (frameInfo.videoType == webrtc::VideoType::kNV12)
	{
		size = width*height * 3 / 2;
	}
	else if (frameInfo.videoType == webrtc::VideoType::kARGB)
	{
		size = width*height * 4;
	}

	int stride_y = width;
	int stride_uv = (width + 1) / 2;
	int target_width = abs(width);
	int target_height = abs(height);

	// Convert frame
	rtc::scoped_refptr<webrtc::I420Buffer> buffer = webrtc::I420Buffer::Create(
		target_width, target_height, stride_y, stride_uv, stride_uv);

	libyuv::RotationMode rotation_mode = libyuv::kRotate0;

	const int conversionResult = libyuv::ConvertToI420(
		videoFrame, size,
		buffer.get()->MutableDataY(), buffer.get()->StrideY(),
		buffer.get()->MutableDataU(), buffer.get()->StrideU(),
		buffer.get()->MutableDataV(), buffer.get()->StrideV(), 0, 0,
		width, height, target_width, target_height,
		rotation_mode, ConvertVideoType(frameInfo.videoType));

	// not using the result yet, silence compiler
	(void)conversionResult;

//	const int64_t obs_timestamp_us = (int64_t)frame->timestamp / rtc::kNumNanosecsPerMicrosec;

	// Align timestamps from OBS capturer with rtc::TimeMicros timebase
//	const int64_t aligned_timestamp_us = timestamp_aligner_.TranslateTimestamp(obs_timestamp_us, rtc::TimeMicros());

	// Create a webrtc::VideoFrame to pass to the capturer
	webrtc::VideoFrame video_frame =
		webrtc::VideoFrame::Builder()
		.set_video_frame_buffer(buffer)
		.set_rotation(webrtc::kVideoRotation_0)
		.set_update_rect(webrtc::VideoFrame::UpdateRect{0,0,width,height})
//		.set_timestamp_us(aligned_timestamp_us)
		.set_id(++frame_id)
		.build();

	// Send frame to video capturer
	rtc::AdaptedVideoTrackSource::OnFrame(video_frame);
}
