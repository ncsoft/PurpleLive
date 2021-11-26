#pragma once
/*
#include "modules/video_capture/video_capture_impl.h"
#include "api/video/i420_buffer.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "modules/include/module_common_types.h"
#include "modules/video_capture/video_capture_config.h"
#include "modules/video_capture/video_capture_impl.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_count.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/time_utils.h"
#include "rtc_base/trace_event.h"
#include "system_wrappers/include/clock.h"
#include "third_party/libyuv/include/libyuv.h"

class VideoCaptureModule : public rtc::RefCountedObject<webrtc::videocapturemodule::VideoCaptureImpl>
{
public:
	VideoCaptureModule()
	{
		started = false;
	}
	int32_t StartCapture(const webrtc::VideoCaptureCapability& capability) override
	{
		_requestedCapability = capability;
		started = true;
		return 0;
	}
	int32_t StopCapture() override 
	{
		started = false;
		return 0;
	}
	bool CaptureStarted() override 
	{ 
		return started;
	}
	int32_t CaptureSettings(webrtc::VideoCaptureCapability& settings) override
	{
		return 0;
	}

	int32_t IncomingFrameEx(uint8_t* videoFrame, size_t videoFrameLength, 
		const webrtc::VideoCaptureCapability& frameInfo, int64_t captureTime =0)
	{
		rtc::CritScope cs(&_apiCs);

		const int32_t width = frameInfo.width;
		const int32_t height = frameInfo.height;

		TRACE_EVENT1("webrtc", "VC::IncomingFrame", "capture_time", captureTime);

		// Not encoded, convert to I420.
		if (frameInfo.videoType != webrtc::VideoType::kMJPEG &&
			CalcBufferSize(frameInfo.videoType, width, abs(height)) !=
			videoFrameLength) {
			RTC_LOG(LS_ERROR) << "Wrong incoming frame length.";
			return -1;
		}

		int stride_y = width;
		int stride_uv = (width + 1) / 2;
		int target_width = width;
		int target_height = height;

		// SetApplyRotation doesn't take any lock. Make a local copy here.
		bool apply_rotation = false;
		webrtc::VideoRotation _rotateFrame = webrtc::VideoRotation::kVideoRotation_0;
		if (apply_rotation) 
		{
			// Rotating resolution when for 90/270 degree rotations.
			if (_rotateFrame == webrtc::VideoRotation::kVideoRotation_90 ||
				_rotateFrame == webrtc::VideoRotation::kVideoRotation_270) {
				target_width = abs(height);
				target_height = width;
			}
		}

		// Setting absolute height (in case it was negative).
		// In Windows, the image starts bottom left, instead of top left.
		// Setting a negative source height, inverts the image (within LibYuv).

		// TODO(nisse): Use a pool?
		rtc::scoped_refptr<webrtc::I420Buffer> buffer = webrtc::I420Buffer::Create(
			target_width, abs(target_height), stride_y, stride_uv, stride_uv);

		libyuv::RotationMode rotation_mode = libyuv::kRotate0;
		if (apply_rotation) {
			switch (_rotateFrame) {
			case webrtc::VideoRotation::kVideoRotation_0:
				rotation_mode = libyuv::kRotate0;
				break;
			case webrtc::VideoRotation::kVideoRotation_90:
				rotation_mode = libyuv::kRotate90;
				break;
			case webrtc::VideoRotation::kVideoRotation_180:
				rotation_mode = libyuv::kRotate180;
				break;
			case webrtc::VideoRotation::kVideoRotation_270:
				rotation_mode = libyuv::kRotate270;
				break;
			}
		}

		const int conversionResult = libyuv::ConvertToI420(
			videoFrame, videoFrameLength, buffer.get()->MutableDataY(),
			buffer.get()->StrideY(), buffer.get()->MutableDataU(),
			buffer.get()->StrideU(), buffer.get()->MutableDataV(),
			buffer.get()->StrideV(), 0, 0,  // No Cropping
			width, height, target_width, target_height, rotation_mode,
			ConvertVideoType(frameInfo.videoType));
		if (conversionResult < 0) {
			RTC_LOG(LS_ERROR) << "Failed to convert capture frame from type "
				<< static_cast<int>(frameInfo.videoType) << "to I420.";
			return -1;
		}

		webrtc::VideoFrame captureFrame(buffer, 0, rtc::TimeMillis(),
			!apply_rotation ? _rotateFrame : webrtc::VideoRotation::kVideoRotation_0);
		captureFrame.set_ntp_time_ms(captureTime);

		DeliverCapturedFrame(captureFrame);

		return 0;
	}
private:
	bool started;
};
*/