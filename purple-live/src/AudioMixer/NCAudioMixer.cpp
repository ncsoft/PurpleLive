#include "NCAudioMixer.h"
#include "AppUtils.h"
#include "AppConfig.h"
#include "easylogging++.h"

NCAudioMixer::NCAudioMixer()
{

}

NCAudioMixer::~NCAudioMixer()
{
	SetEnableMonitoring(false);
	SetEnableAudioDeviceAll(0, 0);
	SetEnableAudioDeviceAll(0, 1);
	elogd("[NCAudioMixer::~NCAudioMixer] Singleton");
}

void MixerAudioCallback(audio_frame_data* frame, void* user_data)
{
	NCAudioMixer* pThis = (NCAudioMixer*)user_data;
	if (pThis->m_capture_callback==nullptr)
		return;

	pThis->m_capture_callback(frame, pThis->m_capture_user_data);
}

void NCAudioMixer::RegisterCaptureCallback(AUDIO_CAPTURE_CALLBACK_FUNC callback, void* user_data)
{
	m_capture_callback = callback;
	m_capture_user_data = user_data;
	m_miniaudio_control.RegisterCaptureCallback(MixerAudioCallback, this);
}

void NCAudioMixer::RegisterMagnitudeCallback(AUDIO_MAGNITUDE_CALLBACK_FUNC callback, void* user_data)
{
	m_miniaudio_control.RegisterMagnitudeCallback(callback, user_data);
}

bool NCAudioMixer::IsMixing()
{
	return m_miniaudio_control.IsMixing();
}

float NCAudioMixer::GetMixerVolume()
{
	return m_miniaudio_control.GetMixerVolume();
}

void NCAudioMixer::SetMixerVolume(float volume)
{
	m_miniaudio_control.SetMixerVolume(volume);
}

bool NCAudioMixer::GetMute(bool input)
{
	return m_miniaudio_control.GetMute(input);
}

void NCAudioMixer::SetMute(bool input, bool mute)
{
	m_miniaudio_control.SetMute(input, mute);
}

float NCAudioMixer::GetInputCaptureVolume()
{
	return m_miniaudio_control.GetCaptureVolume(true);
}

void NCAudioMixer::SetInputCaptureVolume(float volume)
{
	m_miniaudio_control.SetCaptureVolume(true, volume);
}

float NCAudioMixer::GetOutputCaptureVolume()
{
	return m_miniaudio_control.GetCaptureVolume(false);
}

void NCAudioMixer::SetOutputCaptureVolume(float volume)
{
	m_miniaudio_control.SetCaptureVolume(false, volume);
}

bool NCAudioMixer::GetEnableInputCapture()
{
	return m_enable_input_capture;
}

void NCAudioMixer::SetEnableInputCapture(bool enable)
{
	bool is_mixing = IsMixing();
	if (is_mixing)
		Stop();

	m_enable_input_capture = enable;
	
	if (is_mixing)
		Start();
}

void NCAudioMixer::SetEnableOutputCapture(bool enable)
{
	bool is_mixing = IsMixing();
	if (is_mixing)
		Stop();

	m_enable_output_capture = enable;
	
	if (is_mixing)
		Start();
}

void NCAudioMixer::SetEnableMeasureMagnitude(bool enable, bool is_input)
{
	m_miniaudio_control.SetEnableMeasureMagnitude(enable, is_input);
}

void NCAudioMixer::SetEnableAudioDevice(bool enable, bool is_input, int index)
{
	if (index==DEFUALT_DEVICE_INDEX)
	{
		bool& default_selected = (is_input) ? m_default_input_selected : m_default_output_selected;
		default_selected = enable;
		return;
	}

	std::vector<AudioDeviceInfo>& device_list = (is_input) ? m_input_device_info_list : m_input_device_info_list;
	if (index >= device_list.size())
		return;

	return SetEnableAudioDevice(enable, is_input, device_list[index].id);
}

void NCAudioMixer::SetEnableAudioDevice(bool enable, bool is_input, std::string device_id)
{
	std::vector<std::string>& selected_list = (is_input) ? m_selected_input_device_list : m_selected_output_device_list;
	
	if (enable)
	{
		for (int i=0; i<selected_list.size(); i++)
		{
			if (device_id==selected_list[i])
				return;
		}
		selected_list.push_back(device_id);
	}
	else
	{
		for (int i=0; i<selected_list.size(); i++)
		{
			if (device_id==selected_list[i])
			{
				selected_list.erase(begin(selected_list)+i);
				return;
			}
		}
	}
}

void NCAudioMixer::SetEnableAudioDeviceAll(bool enable, bool is_input)
{
	SetEnableAudioDevice(enable, is_input, DEFUALT_DEVICE_INDEX);

	std::vector<std::string>& selected_list = (is_input) ? m_selected_input_device_list : m_selected_output_device_list;
	while (selected_list.size()>0)
	{
		selected_list.erase(begin(selected_list));
	}
}

bool IsCaptureDevice(const MAC_DeviceInfo& mac_device_info, bool default_selected, const std::vector<std::string>& selected_list)
{
	if (default_selected && mac_device_info.is_default)
	{
		return true;
	}

	for (int i=0; i<selected_list.size(); i++)
	{
		if (mac_device_info.id==selected_list[i])
		{
			return true;
		}
	}

	return false;
}

std::vector<AudioDeviceInfo>& NCAudioMixer::GetDeviceInfoList(bool is_input)
{
	std::vector<AudioDeviceInfo>& device_info_list = (is_input) ? m_input_device_info_list : m_output_device_info_list;
	device_info_list.erase(begin(device_info_list), end(device_info_list));

	bool& default_selected = (is_input) ? m_default_input_selected : m_default_output_selected;
	std::vector<std::string>& selected_list = (is_input) ? m_selected_input_device_list : m_selected_output_device_list;

	// make list of AudioDeviceInfo
	std::vector<MAC_DeviceInfo> mac_device_info_list = m_miniaudio_control.GetDeviceList(is_input);
	for (int i=0; i<mac_device_info_list.size(); i++)
	{
		AudioDeviceInfo info;
		info.is_input = mac_device_info_list[i].is_input;
		info.is_default = mac_device_info_list[i].is_default;
		info.is_capture = IsCaptureDevice(mac_device_info_list[i], default_selected, selected_list);
		info.name = mac_device_info_list[i].name;
		info.id = mac_device_info_list[i].id;
		//elogd("[NCAudioMixer::GetDeviceInfoList] input(%d) default(%d) capture(%d) name(%s)", info.is_input, info.is_default, info.is_capture, info.name, info.id);

		device_info_list.push_back(info);
	}
	return device_info_list;
}

void NCAudioMixer::Restart()
{
	Stop();
	Start();
}

void NCAudioMixer::Start()
{
	if (IsMixing())
		Stop();
	
	std::lock_guard<std::mutex> guard(m_mixer_mutex);

	constexpr char* AUDIO_SAVE_PATH = "D:\\data\\media";
	constexpr bool is_dump_audio = false;

	m_current_mac_output_device_info_list = m_miniaudio_control.GetDeviceList(0, true);
	m_current_mac_input_device_info_list = m_miniaudio_control.GetDeviceList(1, true);

	bool is_input;
	std::string save_path = "";

	// capture input/output device
	for(int device_types=0; device_types<2; device_types++)
	{
		is_input = (device_types);
		bool& enable_capture = (is_input) ? m_enable_input_capture : m_enable_output_capture;
		if (!enable_capture) 
			continue;

		bool& default_selected = (is_input) ? m_default_input_selected : m_default_output_selected;
		std::vector<std::string>& selected_list = (is_input) ? m_selected_input_device_list : m_selected_output_device_list;
		std::vector<MAC_DeviceInfo>& mac_device_info_list = (is_input) ? m_current_mac_input_device_info_list : m_current_mac_output_device_info_list;
		for (int i=0; i<mac_device_info_list.size(); i++)
		{
			if (default_selected && mac_device_info_list[i].is_default)
			{
				if (is_dump_audio)
					save_path = AppUtils::format("%s\\ma%d%d.wav", AUDIO_SAVE_PATH, device_types, i).c_str();
				
				mac_device_info_list[i].context = m_miniaudio_control.StartCapture(is_input, i, save_path.c_str());
				continue;
			}

			for (int j=0; j<selected_list.size(); j++)
			{
				if (selected_list[j]==mac_device_info_list[i].id)
				{
					if (is_dump_audio)
						save_path = AppUtils::format("%s\\ma%d%d.wav", AUDIO_SAVE_PATH, device_types, i).c_str();
					
					mac_device_info_list[i].context = m_miniaudio_control.StartCapture(is_input, i, save_path.c_str());
					break;
				}
			}
		}
	}

	if (is_dump_audio)
		save_path = AppUtils::format("%s\\ma90.wav", AUDIO_SAVE_PATH).c_str();

	m_miniaudio_control.StartMixing(save_path.c_str());

	SetEnableMonitoring(true);
}

void NCAudioMixer::Stop()
{
	if (!IsMixing())
		return;
	
	std::lock_guard<std::mutex> guard(m_mixer_mutex);

	SetEnableMonitoring(false);

	m_miniaudio_control.StopMixing();

	for (int i=0; i<m_current_mac_output_device_info_list.size(); i++)
	{
		if (m_current_mac_output_device_info_list[i].context)
		{
			m_miniaudio_control.StopCapture(m_current_mac_output_device_info_list[i].context);
		}
	}

	for (int i=0; i<m_current_mac_input_device_info_list.size(); i++)
	{
		if (m_current_mac_input_device_info_list[i].context)
		{
			m_miniaudio_control.StopCapture(m_current_mac_input_device_info_list[i].context);
		}
	}
}

void NCAudioMixer::SetEnableMonitoring(bool enable)
{
	if (enable==m_enable_monitoring)
		return;

	m_enable_monitoring = enable;

	elogd("[NCAudioMixer::SetEnableMonitoring] enable(%d)", enable);

	if (enable)
	{
		m_monitoring_last = std::chrono::system_clock::now();

		if (m_is_monitoring_thread_started)
			return;		

		m_monitoring_thread = std::thread([](NCAudioMixer* pThis)
		{
			while (true)
			{
				if (pThis->m_is_monitoring_thread_stop)
					break;	
			
				pThis->CheckMonitoring();
				
				std::this_thread::sleep_for(std::chrono::milliseconds(MONITORING_LOOP_INTERVAL));
			}
			pThis->m_is_monitoring_thread_stop = false;
			pThis->m_is_monitoring_thread_started = false;
			//elogd("[NCAudioMixer::SetEnableMonitoring::m_monitoring_thread] finish");
		}, this);
		//m_monitoring_thread.detach();
	}
	else
	{
		m_is_monitoring_thread_stop = true;

		if (m_monitoring_thread.joinable())
			m_monitoring_thread.join();

		//elogd("[NCAudioMixer::SetEnableMonitoring] enable(%d) finish", enable);
	}
}

void NCAudioMixer::CheckMonitoring()
{
	if (!m_enable_monitoring)
		return;

	using namespace std::chrono;
	milliseconds time = duration_cast<milliseconds>(system_clock::now()-m_monitoring_last);
	if (time.count()<MONITORING_INTERVAL)
		return;

	m_monitoring_last = std::chrono::system_clock::now();
	std::thread process_monitoring_thread = std::thread([](NCAudioMixer* pThis)
	{
		pThis->ProcessMonitoring();
	}, this);
	process_monitoring_thread.detach();
}

bool IsLostAudioDevice(std::vector<MAC_DeviceInfo>& org_info_list, std::vector<MAC_DeviceInfo>& new_info_list)
{
	for (int i=0; i<org_info_list.size(); i++)
	{
		if (org_info_list[i].context==nullptr)
			continue;

		bool find = false;
		for (int j=0; j<new_info_list.size(); j++)
		{
			if (org_info_list[i].id==new_info_list[j].id)
			{
				find = true;
				break;
			}
		}
		if (!find)
		{
			return true;
		}
	}
	return false;
}

bool IsChangedDefaultAudioDevice(std::vector<MAC_DeviceInfo>& org_info_list, std::vector<MAC_DeviceInfo>& new_info_list)
{
	int index_org = -1;
	for (int i=0; i<org_info_list.size(); i++)
	{
		if (org_info_list[i].is_default)
		{
			index_org = i;
		}
	}
	
	int index_new = -1;
	for (int i=0; i<new_info_list.size(); i++)
	{
		if (new_info_list[i].is_default)
		{
			index_new = i;
		}
	}

	if (index_org<0 && index_new<0)
	{
		//elogd("[MiniAudioControl::IsChangedDefaultDevice] no default device");
		return false;
	}
	else if (index_org<0 && index_new>=0)
	{
		elogi("[MiniAudioControl::IsChangedDefaultDevice] input(%d) added new default device", new_info_list[index_new].is_input);
		return true;
	}
	else if (index_org>=0 && index_new<0)
	{
		elogi("[MiniAudioControl::IsChangedDefaultDevice] input(%d) lost default device", org_info_list[index_org].is_input);
		return false;
	}
	else
	{
		if (org_info_list[index_org].id!=new_info_list[index_new].id)
		{
			elogi("[MiniAudioControl::IsChangedDefaultDevice] input(%d) changed default device", org_info_list[index_org].is_input);
			return true;
		}
	}

	return false;
}

void NCAudioMixer::ProcessMonitoring()
{
	if (!IsMixing())
		return;

	//elogd("[MiniAudioControl::ProcessMonitoring]");

	bool is_changed_default_audio_device = false;
	// mixer lock brace
	{
		std::lock_guard<std::mutex> guard(m_mixer_mutex);
		std::vector<MAC_DeviceInfo>& output_devices = m_miniaudio_control.GetDeviceList(0);
		std::vector<MAC_DeviceInfo>& input_devices = m_miniaudio_control.GetDeviceList(1);

		for (int device_types=0; device_types<2; device_types++)
		{
			bool is_input = (device_types);
            bool is_default_selected = (is_input) ? m_default_input_selected : m_default_output_selected;
			std::vector<MAC_DeviceInfo>& org_info_list = (is_input) ? m_current_mac_input_device_info_list : m_current_mac_output_device_info_list;
			std::vector<MAC_DeviceInfo>& new_info_list = (is_input) ? input_devices : output_devices;
			
			// monitoring change default audio device
			if (is_default_selected && IsChangedDefaultAudioDevice(org_info_list, new_info_list))
			{
				is_changed_default_audio_device = true;
				break;
			}

			// monitoring lost audio device
			if (IsLostAudioDevice(org_info_list, new_info_list))
			{
				if (!m_monitoring_device_lost)
				{
					m_monitoring_device_lost = true;
					elogi("[MiniAudioControl::ProcessMonitoring] audio device lost");
				}
				//elogd("[MiniAudioControl::ProcessMonitoring] [1] finish");
				return;
			}
		}
	}

	if (is_changed_default_audio_device)
	{
		elogi("[MiniAudioControl::ProcessMonitoring] recover audio device (change)");
		Restart();
	}
	else if (m_monitoring_device_lost)
	{
		m_monitoring_device_lost = false;
		elogi("[MiniAudioControl::ProcessMonitoring] recover audio device (lost)");
		Restart();
	}

	//elogd("[MiniAudioControl::ProcessMonitoring] [2] finish");
}