#pragma once

#include "api/media_stream_interface.h"

struct WebRTCMediaSource
{
	rtc::scoped_refptr<webrtc::AudioSourceInterface> m_audioSource;
	rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> m_videoSource;
};
