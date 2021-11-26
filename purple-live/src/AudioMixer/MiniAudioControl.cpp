#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"

#include "MiniAudioControl.h"
#include "AudioCircularQueue.h"
#include "AppUtils.h"
#include "AppConfig.h"
#include "easylogging++.h"

#include <iostream>
#include <vector>
#include <queue>
#include <math.h>

struct MAC_MixerContext
{
	MAC_MixerContext() { Init(); };
	~MAC_MixerContext() {};

	ma_encoder encoder;
	std::vector<BYTE> mixing_buffer;
	std::vector<BYTE> convert_buffer;

	void Init() {
		memset(&encoder, 0, sizeof(ma_encoder));
	}
};
std::unique_ptr<MAC_MixerContext> g_mixer;
std::mutex g_mixer_mutex;

struct MAC_CaptureContext
{
	MAC_CaptureContext() { Init(); };
	~MAC_CaptureContext() {};

	bool is_input;
	int device_index;

	ma_encoder encoder;
    ma_device device;

	AudioCircularQueue working_queue{MiniAudioControl::CAPTURE_CIRCULAR_QUEUE_SIZE};
	std::vector<BYTE> working_buffer;

	void Init() {
		memset(&encoder, 0, sizeof(ma_encoder));
	}
};
std::vector<std::unique_ptr<MAC_CaptureContext>> g_capture_list;
std::mutex g_capture_mutex;
std::vector<BYTE> g_working_buffer;
bool g_is_mixing = false;

MiniAudioControl::MiniAudioControl()
{

}

MiniAudioControl::~MiniAudioControl()
{
	if (IsMixing())
	{
		StopMixing();
	}

	if (IsCapturing())
	{
		while (g_capture_list.size()>0)
		{
			StopCaptureIndex(0);
		}
	}
	elogd("[MiniAudioControl::~MiniAudioControl] Singleton");
}

std::vector<MAC_DeviceInfo>& MiniAudioControl::GetDeviceList(bool is_input, bool is_log)
{
	ma_result result;
    ma_context context;
    ma_device_info* pPlaybackDeviceInfos;
    ma_uint32 playbackDeviceCount;
    ma_device_info* pCaptureDeviceInfos;
    ma_uint32 captureDeviceCount;
    
	std::vector<MAC_DeviceInfo>& device_list = (is_input) ? m_input_device_list : m_output_device_list;
	device_list.erase(begin(device_list), end(device_list));

    if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
        elogd("[MiniAudioControl::GetDeviceList] Failed to initialize context.");
        return device_list;
    }

    result = ma_context_get_devices(&context, &pPlaybackDeviceInfos, &playbackDeviceCount, &pCaptureDeviceInfos, &captureDeviceCount);
    if (result != MA_SUCCESS) {
        elogd("[MiniAudioControl::GetDeviceList] Failed to retrieve device information.");
        return device_list;
    }

    if (is_input)
	{
		if (is_log)
			elogd("[MiniAudioControl::GetDeviceList] input(%d) count(%d) =======================", is_input, captureDeviceCount);
		for (ma_uint32 iDevice = 0; iDevice < captureDeviceCount; ++iDevice)
		{
			MAC_DeviceInfo device;
			device.is_input = is_input;
			device.is_default = pCaptureDeviceInfos[iDevice].isDefault;
			device.name = StringUtils::utf8_to_string(pCaptureDeviceInfos[iDevice].name);
			device.id = StringUtils::wstring_to_string(pCaptureDeviceInfos[iDevice].id.wasapi);
			if (is_log)
				elogd("\t\t[MiniAudioControl::GetDeviceList] input(%d) index(%d) default(%d) name(%s) id(%s)", is_input, iDevice, device.is_default, device.name.c_str(), device.id.c_str());
			device_list.push_back(device);
		}
	}
	else
	{
		if (is_log)
			elogd("[MiniAudioControl::GetDeviceList] input(%d) count(%d) =======================", is_input, playbackDeviceCount);
		for (ma_uint32 iDevice = 0; iDevice < playbackDeviceCount; ++iDevice)
		{
			MAC_DeviceInfo device;
			device.is_input = is_input;
			device.is_default = pPlaybackDeviceInfos[iDevice].isDefault;
			device.name = StringUtils::utf8_to_string(pPlaybackDeviceInfos[iDevice].name);
			device.id = StringUtils::wstring_to_string(pPlaybackDeviceInfos[iDevice].id.wasapi);
			if (is_log)
				elogd("\t\t[MiniAudioControl::GetDeviceList] input(%d) index(%d) default(%d) name(%s) id(%s)", is_input, iDevice, device.is_default, device.name.c_str(), device.id.c_str());
			device_list.push_back(device);
		}
	}
    
	ma_context_uninit(&context);

    return device_list;
}

void _mixing_capture_data(bool is_input, ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	int buffer_size = frameCount * MiniAudioControl::CAPTURE_BYTE_PER_FRAME;
	int buffer_index = 0;

	// queue capture audio buffer
	for (; buffer_index<g_capture_list.size(); buffer_index++)
	{
		if (pDevice==&g_capture_list[buffer_index]->device)
		{
			g_capture_list[buffer_index]->working_queue.Queue((BYTE*)pInput, buffer_size);
			break;
		}
	}

    (void)pOutput;
}

void mixing_capture_data(bool is_input, ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	std::lock_guard<std::mutex> guard(g_mixer_mutex);

	if (g_is_mixing)
	{
		_mixing_capture_data(is_input, pDevice, pOutput, pInput, frameCount);
	}
}

void simple_input_capture_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	mixing_capture_data(true, pDevice, pOutput, pInput, frameCount);
	
	ma_encoder* pEncoder = (ma_encoder*)pDevice->pUserData;
	if (pEncoder != NULL)
		ma_encoder_write_pcm_frames(pEncoder, pInput, frameCount);
	
	(void)pOutput;
}

void simple_output_capture_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	mixing_capture_data(false, pDevice, pOutput, pInput, frameCount);

	ma_encoder* pEncoder = (ma_encoder*)pDevice->pUserData;
	if (pEncoder != NULL)
		ma_encoder_write_pcm_frames(pEncoder, pInput, frameCount);
    
	(void)pOutput;
}

void MiniAudioControl::RegisterCaptureCallback(AUDIO_CAPTURE_CALLBACK_FUNC callback, void* user_data)
{
	elogd("[MiniAudioControl::RegisterCaptureCallback] callback(0x%x) user_data(0x%x)", callback, user_data);
	m_capture_callback = callback;
	m_capture_user_data = user_data;
}

void MiniAudioControl::RegisterMagnitudeCallback(AUDIO_MAGNITUDE_CALLBACK_FUNC callback, void* user_data)
{
	elogd("[MiniAudioControl::RegisterMagnitudeCallback] callback(0x%x) user_data(0x%x)", callback, user_data);
	m_magnitude_callback = callback;
	m_magnitude_user_data = user_data;
}

bool MiniAudioControl::IsMixing()
{
	return g_is_mixing;
}

bool MiniAudioControl::IsCapturing()
{
	return (g_capture_list.size());
}

bool MiniAudioControl::GetMute(bool input)
{
	return input ? m_input_mute : m_output_mute;
}

void MiniAudioControl::SetMute(bool input, bool mute)
{
	bool& mixer_mute = input ? m_input_mute : m_output_mute;
	mixer_mute = mute;
}

float MiniAudioControl::GetMixerVolume()
{
	return m_mixer_volume;
}

void MiniAudioControl::SetMixerVolume(float volume)
{
	m_mixer_volume = volume;
}

float MiniAudioControl::GetCaptureVolume(bool is_input)
{
	return (is_input) ? m_input_volume : m_output_volume;
}

void MiniAudioControl::SetCaptureVolume(bool is_input, float volume)
{
	if (is_input)
	{
		m_input_volume = volume;
	}
	else
	{
		m_output_volume = volume;
	}
}

void MiniAudioControl::SetEnableMeasureMagnitude(bool enable, bool is_input)
{
	bool& enable_measure_magnitue = is_input ? m_enable_input_magnitude : m_enable_output_magnitude;
	enable_measure_magnitue = enable;
}

void* MiniAudioControl::StartCapture(int is_input, int device_index, const char* save_path)
{
	elogd("[MiniAudioControl::StartCapture] input(%d) index(%d)", is_input, device_index);

	std::unique_ptr<MAC_CaptureContext> capture_context(std::make_unique<MAC_CaptureContext>());

	capture_context->is_input = is_input;
	capture_context->device_index = device_index;

	while (true)
	{
		ma_result result;
		
		// 1. init working queue
		capture_context->working_queue.SetCapability(CAPTURE_CIRCULAR_QUEUE_SIZE);

		// 2. init encoder
		if (save_path!=nullptr && strlen(save_path)>0)
		{
			ma_format format = (CAPTURE_BYTE_PER_SAMPLE==AUDIO_SAMPLE_FORMAT_4BYTE) ? ma_format_f32 : ma_format_s16;
			ma_encoder_config encoderConfig = ma_encoder_config_init(ma_resource_format_wav, format, CAPTURE_CHANNEL_COUNT, CAPTURE_SAMPLE_RATE);
			if (ma_encoder_init_file(save_path, &encoderConfig, &capture_context->encoder) != MA_SUCCESS) {
				eloge("[MiniAudioControl::StartCapture] Failed to initialize output file");
				break;
			}
		}

		// 3. init device
		ma_device_type device_type = is_input ? ma_device_type_capture : ma_device_type_loopback;
		ma_device_config deviceConfig = ma_device_config_init(device_type);
		if (device_index>=0)
		{
			ma_context context;
			ma_device_info* pPlaybackDeviceInfos;
			ma_uint32 playbackDeviceCount;
			ma_device_info* pCaptureDeviceInfos;
			ma_uint32 captureDeviceCount;
			
			if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
				eloge("[MiniAudioControl::StartCapture] Failed to initialize context.");
				break;
			}

			result = ma_context_get_devices(&context, &pPlaybackDeviceInfos, &playbackDeviceCount, &pCaptureDeviceInfos, &captureDeviceCount);
			if (result != MA_SUCCESS) {
				eloge("[MiniAudioControl::StartCapture] Failed to retrieve device information.");
				break;
			}

			if (is_input)
			{
				if (device_index >= (int)captureDeviceCount)
				{
					eloge("[MiniAudioControl::StartCapture] device_index(%d) captureDeviceCount(%d)", device_index, captureDeviceCount);
					break;
				}
				deviceConfig.capture.pDeviceID = &(pCaptureDeviceInfos[device_index].id);
			}
			else
			{
				if (device_index >= (int)playbackDeviceCount)
				{
					eloge("[MiniAudioControl::StartCapture] device_index(%d) playbackDeviceCount(%d)", device_index, playbackDeviceCount);
					break;
				}
				deviceConfig.playback.pDeviceID = &(pPlaybackDeviceInfos[device_index].id);
			}
		}
	
		if (save_path!=nullptr && strlen(save_path)>0)
		{
			deviceConfig.capture.format   = capture_context->encoder.config.format;
			deviceConfig.capture.channels = capture_context->encoder.config.channels;
			deviceConfig.sampleRate       = capture_context->encoder.config.sampleRate;
			deviceConfig.pUserData        = &capture_context->encoder;
		}
		else
		{
			ma_format format = (CAPTURE_BYTE_PER_SAMPLE==AUDIO_SAMPLE_FORMAT_4BYTE) ? ma_format_f32 : ma_format_s16;
			deviceConfig.capture.format   = format;
			deviceConfig.capture.channels = CAPTURE_CHANNEL_COUNT;
			deviceConfig.sampleRate       = CAPTURE_SAMPLE_RATE;
			deviceConfig.pUserData        = NULL;
		}
		deviceConfig.dataCallback     = is_input ? simple_input_capture_data_callback : simple_output_capture_data_callback;

		result = ma_device_init(NULL, &deviceConfig, &capture_context->device);
		if (result != MA_SUCCESS) {
			eloge("[MiniAudioControl::StartCapture] Failed to initialize capture device.");
			break;
		}

		result = ma_device_start(&capture_context->device);
		if (result != MA_SUCCESS) {
			ma_device_uninit(&capture_context->device);
			eloge("[MiniAudioControl::StartCapture] Failed to start device.");
			break;
		}

		std::lock_guard<std::mutex> guard(g_capture_mutex);
		g_capture_list.push_back(std::move(capture_context));
 		return (void*)g_capture_list[g_capture_list.size()-1].get();
	}

	return 0;
}

void MiniAudioControl::StopCaptureIndex(int index)
{
	std::unique_ptr<MAC_CaptureContext>& capture_context = g_capture_list[index];
	{
		ma_device_stop(&(capture_context->device));
		ma_device_uninit(&(capture_context->device));
		ma_encoder_uninit(&(capture_context->encoder));
			
		capture_context->working_buffer.clear();
		std::vector<BYTE>().swap(capture_context->working_buffer);
	}

	g_capture_list.erase(g_capture_list.begin()+index);
}

void MiniAudioControl::StopCapture(void* capture_context)
{
	std::lock_guard<std::mutex> guard_capture(g_capture_mutex);
	
	for (int i=0; i<g_capture_list.size(); i++)
	{
		if (capture_context==(void*)g_capture_list[i].get())
		{
			StopCaptureIndex(i);
			break;
		}
	} 
}

void MiniAudioControl::StartMixing(const char* save_path)
{
	if (g_capture_list.size()==0)
		return;

	if (g_is_mixing)
		return;

	std::lock_guard<std::mutex> guard(g_mixer_mutex);

	g_mixer = std::make_unique<MAC_MixerContext>();

	if (save_path!=nullptr && strlen(save_path)>0)
	{
		ma_format format = (MIXING_BYTE_PER_SAMPLE==AUDIO_SAMPLE_FORMAT_4BYTE) ? ma_format_f32 : ma_format_s16;
		ma_encoder_config encoderConfig = ma_encoder_config_init(ma_resource_format_wav, format, CAPTURE_CHANNEL_COUNT, CAPTURE_SAMPLE_RATE);
		if (ma_encoder_init_file(save_path, &encoderConfig, &g_mixer->encoder) != MA_SUCCESS) {
			eloge("[MiniAudioControl::StartMixing] Failed to initialize output file.");
		}
	}

	// init mixing buffer
	g_working_buffer.resize(MIXING_CHUNKED_SIZE);
	g_mixer->mixing_buffer.resize(MIXING_CHUNKED_SIZE);	
	g_mixer->convert_buffer.resize(MIXING_CHUNKED_SIZE);	
	
	StartMixingThread();

	g_is_mixing = true;
}

void MiniAudioControl::StopMixing()
{
	if (!g_is_mixing)
		return;

	g_is_mixing = false;

	StopMixingThread();

	std::lock_guard<std::mutex> guard(g_mixer_mutex);

	ma_encoder_uninit(&g_mixer->encoder);
	
	g_mixer->Init();
}

static inline float sample_to_decibel(const float sample)
{
	return (sample == 0.0f) ? -INFINITY : (20.0f * log10f(sample));
}

static inline float decibel_to_sample(const float db)
{
	return isfinite((double)db) ? powf(10.0f, db / 20.0f) : 0.0f;
}

float MeasureMagnitudeRate(BYTE* buf, int size, int byte_per_sample)
{
	constexpr bool use_peak = true;	// peak or average, obs(peak)
	constexpr float min_db = -60;	// obs(-60), purple(-100)
	constexpr float max_db = 0;
	constexpr float db_range = max_db-min_db;
	constexpr float normalize_multiplier = 1 / db_range;
	int sample_count = size / byte_per_sample;
	if (byte_per_sample==MiniAudioControl::AUDIO_SAMPLE_FORMAT_4BYTE)
	{
		float sum = 0;
		float peak = 0;
		float curr = 0;
		float* source = (float*)buf;
		for (int x=0; x<sample_count; x++)
		{
			curr = fabs(source[x]);
			sum += curr;
			if (peak < curr) {
				peak = curr;
			}
		}
		float avg_db = sample_to_decibel(sum/sample_count);
		float peak_db = sample_to_decibel(peak);
		float db = use_peak ? peak_db : avg_db;
		float magnitude_rate = (db + db_range) * normalize_multiplier;
		magnitude_rate = min(1.0f, max(0.0f, magnitude_rate));
		//elogd("[MiniAudioControl::MeasureMagnitudeRate] magnitude_rate(%f) avg_db(%f) peak_db(%f)", magnitude_rate, avg_db, peak_db);
		return magnitude_rate;
	}
	else if (byte_per_sample==MiniAudioControl::AUDIO_SAMPLE_FORMAT_2BYTE)
	{
		int sum = 0;
		int16_t peak = 0;
		int16_t curr = 0;
		int16_t* source = (int16_t*)buf;
		for (int x=0; x<sample_count; x++)
		{
			curr = abs(source[x]);
			sum += curr;
			if (peak < curr)
				peak = curr;
		}
		float avg_db = sample_to_decibel(((float)sum/sample_count)/0x7FFF);
		float peak_db = sample_to_decibel((float)peak/0x7FFF);
		float db = use_peak ? peak_db : avg_db;
		float magnitude_rate = (db + db_range) * normalize_multiplier;
		magnitude_rate = min(1.0f, max(0.0f, magnitude_rate));
		//elogd("[MiniAudioControl::MeasureMagnitudeRate] magnitude_rate(%f) avg(%3d) peak(%3d) avg_db(%f) peak_db(%f)", magnitude_rate, sum/samples, peak, avg_db, peak_db);
		return magnitude_rate;
	}

	return 0.0f;
}

void AudioVolume(BYTE* buf, int size, float volume, int byte_per_sample)
{
	if (volume>=1.0)
		return;

	float volume_multiplier = volume;
	//static const float ONE_PER_TAN1 = 1/tan(1.0f);
	//float volume_multiplier = tan(volume)*ONE_PER_TAN1;

	int sample_count = size/byte_per_sample;
	if (byte_per_sample==MiniAudioControl::AUDIO_SAMPLE_FORMAT_4BYTE)
	{
		float* source = (float*)buf;
		for (int x=0; x<sample_count; x++)
		{
			source[x] *= volume_multiplier;
		}
	}
	else if (byte_per_sample==MiniAudioControl::AUDIO_SAMPLE_FORMAT_2BYTE)
	{
		uint16_t* source = (uint16_t*)buf;
		for (int x=0; x<sample_count; x++)
		{
			source[x] *= volume_multiplier;
		}
	}
}

void AudioMixing(BYTE* mixing_buf, BYTE* working_buf, int size, int byte_per_sample)
{
	// mixing audio raw data
	int sample_count = size/byte_per_sample;
	if (byte_per_sample==MiniAudioControl::AUDIO_SAMPLE_FORMAT_4BYTE)
	{
		float* target = (float*)mixing_buf;
		float* source = (float*)working_buf;

		for (int x=0; x<sample_count; x++)
		{
			target[x] += source[x];
		}
	}
	else if (byte_per_sample==MiniAudioControl::AUDIO_SAMPLE_FORMAT_2BYTE)
	{
		uint16_t* target = (uint16_t*)mixing_buf;
		uint16_t* source = (uint16_t*)working_buf;

		for (int x=0; x<sample_count; x++)
		{
			target[x] += source[x];
		}
	}
}

bool MiniAudioControl::ProcessAudioMixing()
{
	std::lock_guard<std::mutex> guard(g_mixer_mutex);

	if (m_is_mixing_thread_stop)
		return false;
		
	// 1. check capture buffer size
	bool need_mixing = g_capture_list.size()>0;
	bool need_overflow = false;
	for (int i=0; i<g_capture_list.size(); i++)
	{
		int queue_size = (int)g_capture_list[i]->working_queue.GetSize();
			
		if (queue_size < MIXING_CHUNKED_SIZE)
			need_mixing = false;
			
		if (queue_size >= CAPTURE_QUEUE_OVERFLOW_SIZE)
			need_overflow = true;
	}

	if (!need_mixing && !need_overflow)
	{
		return false;
	}

	// 2. mixing capture buffer list
	for (int i=0; i<g_capture_list.size(); i++)
	{
		memset(g_working_buffer.data(), 0, MIXING_CHUNKED_SIZE);
				
		int queue_size = (int)g_capture_list[i]->working_queue.GetSize();
		int working_pos = 0;
		int read_size = MIXING_CHUNKED_SIZE; 
		if (queue_size < MIXING_CHUNKED_SIZE)
		{
			working_pos = MIXING_CHUNKED_SIZE-queue_size;
			read_size = queue_size;
		}
				
		if (queue_size > 0)
		{
			g_capture_list[i]->working_queue.Dequeue(&g_working_buffer.data()[working_pos], read_size);
		}

		// 2-1. capture audio volume change
		float device_mute = (g_capture_list[i]->is_input) ? m_input_mute : m_output_mute;
		float device_volume = (g_capture_list[i]->is_input) ? m_input_volume : m_output_volume;
		device_volume = (device_mute) ? 0 : device_volume;
		AudioVolume(g_working_buffer.data(), MIXING_CHUNKED_SIZE, device_volume, CAPTURE_BYTE_PER_SAMPLE);

		// 2-2. audio magnitude callback
		bool& enable_measure_magnitue = (g_capture_list[i]->is_input) ? m_enable_input_magnitude : m_enable_output_magnitude;
		if (m_magnitude_callback && enable_measure_magnitue)
		{
			audio_magnitude info;
			info.is_input = g_capture_list[i]->is_input;
			info.device_index = g_capture_list[i]->device_index;
			info.magnitude_rate = MeasureMagnitudeRate(g_working_buffer.data(), MIXING_CHUNKED_SIZE, CAPTURE_BYTE_PER_SAMPLE);
			m_magnitude_callback(&info, m_magnitude_user_data);
		}

		if (i==0)
		{
			// 2-3. first audio raw data
			memcpy(g_mixer->mixing_buffer.data(), g_working_buffer.data(), MIXING_CHUNKED_SIZE);
		}
		else
		{
			// 2-4. mixing audio raw data
			AudioMixing(g_mixer->mixing_buffer.data(), g_working_buffer.data(), MIXING_CHUNKED_SIZE, CAPTURE_BYTE_PER_SAMPLE);
		}
	}

	// 3. mixing audio volume change
	AudioVolume(g_mixer->mixing_buffer.data(), MIXING_CHUNKED_SIZE, m_mixer_volume, CAPTURE_BYTE_PER_SAMPLE);

    // 4. convert sample format
    if (CAPTURE_BYTE_PER_SAMPLE != MIXING_BYTE_PER_SAMPLE)
    {
        ma_format formatOut = (MIXING_BYTE_PER_SAMPLE==AUDIO_SAMPLE_FORMAT_4BYTE) ? ma_format_f32 : ma_format_s16;
        ma_format formatIn = (CAPTURE_BYTE_PER_SAMPLE==AUDIO_SAMPLE_FORMAT_4BYTE) ? ma_format_f32 : ma_format_s16;
        ma_convert_pcm_frames_format(g_mixer->convert_buffer.data(), formatOut, g_mixer->mixing_buffer.data(), formatIn, MIXING_FRAME_SAMPLES, CAPTURE_CHANNEL_COUNT, ma_dither_mode_none);
        memcpy(g_mixer->mixing_buffer.data(), g_mixer->convert_buffer.data(), MIXING_CHUNKED_SIZE);
    }

	// 5. mixing complete callback
	if (m_capture_callback)
	{
		audio_frame_data frame;
		frame.data = g_mixer->mixing_buffer.data();
		frame.frames = MIXING_FRAME_SAMPLES;
		m_capture_callback(&frame, m_capture_user_data);
	}

	// 6. save wav file
	ma_encoder* pEncoder = (ma_encoder*)&g_mixer->encoder;
	if (pEncoder && pEncoder->onWritePCMFrames)
	{
		ma_encoder_write_pcm_frames(pEncoder, g_mixer->mixing_buffer.data(), MIXING_FRAME_SAMPLES);
	}

	return true;
}

void MiniAudioControl::StartMixingThread()
{
	if (m_is_mixing_thread_started)
		return;

	elogd("[MiniAudioControl::StartMixingThread]");

	m_mixing_thread = std::thread([](MiniAudioControl* pThis)
	{
		while (true)
		{
			if (pThis->m_is_mixing_thread_stop)
				break;	
			
			if (pThis->ProcessAudioMixing())
				continue;

			std::this_thread::sleep_for(std::chrono::milliseconds(MiniAudioControl::MIXING_WAIT_TIME));
		}
		pThis->m_is_mixing_thread_stop = false;
		pThis->m_is_mixing_thread_started = false;
		//elogd("[MiniAudioControl::StartMixingThread::m_mixing_thread] finish");
	}, this);

	//m_mixing_thread.detach();
}

void MiniAudioControl::StopMixingThread()
{
	elogd("[MiniAudioControl::StopMixingThread]");

	m_is_mixing_thread_stop = true;

	if (m_mixing_thread.joinable())
		m_mixing_thread.join();

	//elogd("[MiniAudioControl::StopMixingThread] finish");
}