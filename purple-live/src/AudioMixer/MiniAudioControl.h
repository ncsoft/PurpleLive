#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include "NCCapture.h"

// Audio Magnitude Callback Interface
struct audio_magnitude {
	bool is_input;
	int device_index;
	float magnitude_rate;	// 0.0f ~ 1.0f
};
typedef void(*AUDIO_MAGNITUDE_CALLBACK_FUNC)(audio_magnitude*, void*);

// Audio Callback Interface
struct audio_frame_data {
	unsigned char    *data;
	unsigned int    frames;
};
typedef void(*AUDIO_CAPTURE_CALLBACK_FUNC)(audio_frame_data*, void*);

struct MAC_DeviceInfo
{
	bool is_input = false;
	bool is_default = false;

	std::string name = "";
	std::string id = "";
	void* context = nullptr;
};

class MiniAudioControl : public NCCaptureSDK::Singleton<NCCapture>
{
public:
	MiniAudioControl();
	virtual ~MiniAudioControl();

	static constexpr int DEFAULT_DEVICE_INDEX = -1;

	static constexpr int AUDIO_SAMPLE_FORMAT_2BYTE = 2;
	static constexpr int AUDIO_SAMPLE_FORMAT_4BYTE = 4;

	static constexpr int CAPTURE_SAMPLE_RATE = 48000;
	static constexpr int CAPTURE_CHANNEL_COUNT = 2;
	
	static constexpr int CAPTURE_BYTE_PER_SAMPLE = AUDIO_SAMPLE_FORMAT_4BYTE;	// mixing audio format (4 byte float)
	static constexpr int CAPTURE_BYTE_PER_FRAME = CAPTURE_BYTE_PER_SAMPLE * CAPTURE_CHANNEL_COUNT;

	static constexpr int MIXING_BYTE_PER_SAMPLE = AUDIO_SAMPLE_FORMAT_2BYTE;	// media-sdk audio sample format (2 byte)
	static constexpr int MIXING_WAIT_TIME = 10;				// 10ms
	static constexpr int MIXING_FRAME_SAMPLES = 1024;		
	static constexpr int MIXING_CHUNKED_SIZE = 1024 * CAPTURE_BYTE_PER_FRAME;

	static constexpr int CAPTURE_CIRCULAR_QUEUE_SIZE = 4096 * CAPTURE_BYTE_PER_FRAME;
	static constexpr int CAPTURE_QUEUE_OVERFLOW_SIZE = 2048 * CAPTURE_BYTE_PER_FRAME;

	void RegisterCaptureCallback(AUDIO_CAPTURE_CALLBACK_FUNC callback, void* user_data);
	void RegisterMagnitudeCallback(AUDIO_MAGNITUDE_CALLBACK_FUNC callback, void* user_data);

	bool IsMixing();
	bool IsCapturing();
	
	bool GetMute(bool input);
	void SetMute(bool input, bool mute);

	float GetMixerVolume();
	void SetMixerVolume(float volume);
	
	float GetCaptureVolume(bool is_input);
	void SetCaptureVolume(bool is_input, float volume);

	void SetEnableMeasureMagnitude(bool enable, bool is_input);	

	std::vector<MAC_DeviceInfo>& GetDeviceList(bool is_input, bool is_log = false);

	void* StartCapture(int is_input, int device_index, const char* save_path=nullptr);
	void StopCapture(void* capture_context);

	void StartMixing(const char* save_path=nullptr);
	void StopMixing();

protected:
	void StopCaptureIndex(int index);

	bool ProcessAudioMixing();
	void StartMixingThread();
	void StopMixingThread();

	AUDIO_CAPTURE_CALLBACK_FUNC		m_capture_callback = nullptr;
	void*							m_capture_user_data = nullptr;

	AUDIO_MAGNITUDE_CALLBACK_FUNC	m_magnitude_callback = nullptr;
	void*							m_magnitude_user_data = nullptr;

	bool							m_enable_input_magnitude = false;
	bool							m_enable_output_magnitude = false;

	std::vector<MAC_DeviceInfo>		m_input_device_list;
	std::vector<MAC_DeviceInfo>		m_output_device_list;
	std::vector<MAC_DeviceInfo>		m_temp_device_list;
	
	float							m_mixer_volume = 1.0f;
	float							m_input_volume = 1.0f;
	float							m_output_volume = 1.0f;

	bool							m_input_mute = false;
	bool							m_output_mute = false;

	std::thread						m_mixing_thread;
	bool							m_is_mixing_thread_stop = false;
	bool							m_is_mixing_thread_started = false;
};
