#pragma once

#include "MiniAudioControl.h"

struct AudioDeviceInfo
{
	bool is_input = false;
	bool is_default = false;
	bool is_capture = false;
	std::string name = "";
	std::string id = "";
};

class NCAudioMixer : public NCCaptureSDK::Singleton<NCAudioMixer>
{
public:
	NCAudioMixer();
	virtual ~NCAudioMixer();

	static constexpr int MONITORING_INTERVAL = 5000;
	static constexpr int MONITORING_LOOP_INTERVAL = 200;
	static constexpr int DEFUALT_DEVICE_INDEX = -1;
	
	void RegisterCaptureCallback(AUDIO_CAPTURE_CALLBACK_FUNC callback, void* user_data);
	void RegisterMagnitudeCallback(AUDIO_MAGNITUDE_CALLBACK_FUNC callback, void* user_data);

	bool IsMixing();
	
	float GetMixerVolume();
	void SetMixerVolume(float volume);
	
	bool GetMute(bool input);
	void SetMute(bool input, bool mute);

	float GetInputCaptureVolume();
	void SetInputCaptureVolume(float volume);

	float GetOutputCaptureVolume();
	void SetOutputCaptureVolume(float volume);

	bool GetEnableInputCapture();
	void SetEnableInputCapture(bool enable);
	void SetEnableOutputCapture(bool enable);

	void SetEnableMeasureMagnitude(bool enable, bool is_input);
	std::vector<AudioDeviceInfo>& GetDeviceInfoList(bool is_input);

	void SetEnableAudioDevice(bool enable, bool is_input, int index);
	void SetEnableAudioDevice(bool enable, bool is_input, std::string device_id);
	void SetEnableAudioDeviceAll(bool enable, bool is_input);
	
	void Restart();
	void Start();
	void Stop();

	void SetEnableMonitoring(bool enable);
	void CheckMonitoring();
	void ProcessMonitoring();

public:
	MiniAudioControl m_miniaudio_control;

	AUDIO_CAPTURE_CALLBACK_FUNC m_capture_callback = nullptr;
	void* m_capture_user_data = nullptr;

	bool m_enable_input_capture = true;
	bool m_enable_output_capture = true;

	std::vector<AudioDeviceInfo> m_input_device_info_list;
	std::vector<AudioDeviceInfo> m_output_device_info_list;

	bool m_default_input_selected = false;
	bool m_default_output_selected = false;

	std::vector<std::string> m_selected_input_device_list;
	std::vector<std::string> m_selected_output_device_list;

	std::vector<MAC_DeviceInfo> m_current_mac_input_device_info_list;
	std::vector<MAC_DeviceInfo> m_current_mac_output_device_info_list;
	std::mutex m_mixer_mutex;

	bool m_enable_monitoring = false;
	std::chrono::system_clock::time_point m_monitoring_last;
	bool m_monitoring_device_lost = false;

	std::thread m_monitoring_thread;
	bool m_is_monitoring_thread_stop = false;
	bool m_is_monitoring_thread_started = false;
};
