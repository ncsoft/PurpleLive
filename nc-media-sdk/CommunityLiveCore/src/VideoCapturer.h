#pragma once

#include "media/base/adapted_video_track_source.h"
#include "modules/video_capture/video_capture.h"
#include "rtc_base/thread.h"
#include "rtc_base/timestamp_aligner.h"

#include <mutex>

class VideoCapturer : public rtc::AdaptedVideoTrackSource 
{
public:
	VideoCapturer();
	~VideoCapturer() override;

	void OnFrameCaptured(uint8_t* videoFrame, const webrtc::VideoCaptureCapability& frameInfo, int64_t captureTime = 0);

	// VideoTrackSourceInterface API
	bool is_screencast() const override { return false; }
	absl::optional<bool> needs_denoising() const override { return false; }

	// MediaSourceInterface API
	webrtc::MediaSourceInterface::SourceState state() const override
	{
		return webrtc::MediaSourceInterface::SourceState::kLive;
	}
	bool remote() const override { return false; }
private:
	rtc::Thread* start_thread_;
	std::mutex mutex;
	rtc::TimestampAligner timestamp_aligner_;
	uint16_t frame_id=0;
};