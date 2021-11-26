#pragma once

#include <stdio.h>

#include "modules/audio_device/audio_device_generic.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/critical_section.h"
#include "rtc_base/checks.h"

using webrtc::AudioDeviceBuffer;
using webrtc::AudioDeviceGeneric;
using webrtc::AudioDeviceModule;
using webrtc::AudioTransport;
using webrtc::kAdmMaxDeviceNameSize;
using webrtc::kAdmMaxGuidSize;
using webrtc::kAdmMaxFileNameSize;

class AudioDeviceModuleWrapper 
	: public rtc::RefCountedObject<AudioDeviceModule>
{
public:
	AudioDeviceModuleWrapper();
	~AudioDeviceModuleWrapper();

	virtual int64_t TimeUntilNextProcess() { return 1000; }
	virtual void Process() {}

	// Retrieve the currently utilized audio layer
	virtual int32_t ActiveAudioLayer(AudioLayer* audioLayer) const 
	{ 
		*audioLayer = AudioLayer::kDummyAudio;
		return 0;
	}

	// Full-duplex transportation of PCM audio
	virtual int32_t RegisterAudioCallback(AudioTransport* audioTransport)
	{
		this->audioTransport = audioTransport;
		return 0;
	}

	// Main initialization and termination
	virtual int32_t Init();
	virtual int32_t Terminate();
	virtual bool Initialized() const;

	// Device enumeration
	virtual int16_t PlayoutDevices() {  return 0; }
	virtual int16_t RecordingDevices() { return 1; }
	virtual int32_t PlayoutDeviceName(uint16_t index, char name[kAdmMaxDeviceNameSize],char guid[kAdmMaxGuidSize]) 	{ return 0; }
	virtual int32_t RecordingDeviceName(uint16_t index,char name[kAdmMaxDeviceNameSize],char guid[kAdmMaxGuidSize]) 
	{
		sprintf_s(name, kAdmMaxDeviceNameSize, "nc_stream");
		sprintf_s(name, kAdmMaxDeviceNameSize, "obs");
		return 0;
	}

	// Device selection
	virtual int32_t SetPlayoutDevice(uint16_t index) { return 0; }
	virtual int32_t SetPlayoutDevice(WindowsDeviceType device) { return 0; }
	virtual int32_t SetRecordingDevice(uint16_t index) { return 0; }
	virtual int32_t SetRecordingDevice(WindowsDeviceType device) { return 0; }

	// Audio transport initialization
	virtual int32_t PlayoutIsAvailable(bool* available) { return 0; }
	virtual int32_t InitPlayout() { return 0; }
	virtual bool PlayoutIsInitialized() const { return 0; }
	virtual int32_t RecordingIsAvailable(bool* available) { return 0; }
	virtual int32_t InitRecording() { return 0; }
	virtual bool RecordingIsInitialized() const { return 0;  }

	// Audio transport control
	virtual int32_t StartPlayout() { return 0; }
	virtual int32_t StopPlayout() { return 0; }
	virtual bool Playing() const { return 0; }
	virtual int32_t StartRecording() { return 0; }
	virtual int32_t StopRecording() { return 0; }
	virtual bool Recording() const { return true; }

	// Audio mixer initialization
	virtual int32_t InitSpeaker() { return 0;  }
	virtual bool SpeakerIsInitialized() const { return 0; }
	virtual int32_t InitMicrophone() { return 0; }
	virtual bool MicrophoneIsInitialized() const { return true; }

	// Speaker volume controls
	virtual int32_t SpeakerVolumeIsAvailable(bool* available) { return 0; }
	virtual int32_t SetSpeakerVolume(uint32_t volume) { return 0; }
	virtual int32_t SpeakerVolume(uint32_t* volume) const { return 0; }
	virtual int32_t MaxSpeakerVolume(uint32_t* maxVolume) const { return 0; }
	virtual int32_t MinSpeakerVolume(uint32_t* minVolume) const { return 0; }

	// Microphone volume controls
	virtual int32_t MicrophoneVolumeIsAvailable(bool* available) { return 0; }
	virtual int32_t SetMicrophoneVolume(uint32_t volume) { return 0; }
	virtual int32_t MicrophoneVolume(uint32_t* volume) const { return 0; }
	virtual int32_t MaxMicrophoneVolume(uint32_t* maxVolume) const { return 0; }
	virtual int32_t MinMicrophoneVolume(uint32_t* minVolume) const { return 0; }

	// Speaker mute control
	virtual int32_t SpeakerMuteIsAvailable(bool* available) { return 0; }
	virtual int32_t SetSpeakerMute(bool enable) { return 0; }
	virtual int32_t SpeakerMute(bool* enabled) const { return 0; }

	// Microphone mute control
	virtual int32_t MicrophoneMuteIsAvailable(bool* available) { return 0; }
	virtual int32_t SetMicrophoneMute(bool enable) { return 0; }
	virtual int32_t MicrophoneMute(bool* enabled) const { return 0; }

	// Stereo support
	virtual int32_t StereoPlayoutIsAvailable(bool* available) const { return 0; }
	virtual int32_t SetStereoPlayout(bool enable) { return 0; }
	virtual int32_t StereoPlayout(bool* enabled) const { return 0; }
	virtual int32_t StereoRecordingIsAvailable(bool* available) const { return 0; }
	virtual int32_t SetStereoRecording(bool enable) { return 0; }
	virtual int32_t StereoRecording(bool* enabled) const { return 0; }

	// Playout delay
	virtual int32_t PlayoutDelay(uint16_t* delayMS) const { return 0; }

	// Only supported on Android.
	virtual bool BuiltInAECIsAvailable() const { return false; }
	virtual bool BuiltInAGCIsAvailable() const { return false; }
	virtual bool BuiltInNSIsAvailable() const { return false; }

	// Enables the built-in audio effects. Only supported on Android.
	virtual int32_t EnableBuiltInAEC(bool enable) { return 0; }
	virtual int32_t EnableBuiltInAGC(bool enable) { return 0; }
	virtual int32_t EnableBuiltInNS(bool enable) { return 0; }


	void onIncomingData(uint8_t* data, size_t samples_per_channel);

public:
	bool _initialized;
	rtc::CriticalSection _critSect;
	AudioTransport* audioTransport;
	uint8_t pending[640 * 2 * 2];
	size_t pendingLength;
};
