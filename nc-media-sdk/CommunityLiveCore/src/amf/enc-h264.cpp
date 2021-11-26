/*
 * A Plugin that integrates the AMD AMF encoder into OBS Studio
 * Copyright (C) 2016 - 2018 Michael Fabian Dirks
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "enc-h264.hpp"
#include "amf-capabilities.hpp"
#include "amf-encoder-h264.hpp"
#include "strings.hpp"
#include "utility.hpp"

#define PREFIX "[H264/AVC]"

using namespace Plugin;
using namespace Plugin::AMD;
using namespace Plugin::Interface;
using namespace Utility;

Plugin::Interface::H264Interface::H264Interface(const AmfEncoderConfig& config)
{
	PLOG_DEBUG("<" __FUNCTION_NAME__ "> Initializing...");

	init(config);

	PLOG_DEBUG("<" __FUNCTION_NAME__ "> Complete.");
}

Plugin::Interface::H264Interface::~H264Interface()
{
	PLOG_DEBUG("<" __FUNCTION_NAME__ "> Finalizing...");

	if (m_VideoEncoder) {
		m_VideoEncoder->Stop();
		m_VideoEncoder = nullptr;
	}

	PLOG_DEBUG("<" __FUNCTION_NAME__ "> Complete.");
}

void Plugin::Interface::H264Interface::open(const AmfEncoderConfig& config)
{
	bool debug = true;
	Plugin::AMD::AMF::Instance()->EnableDebugTrace(debug);
	
	update(config);

	try {
		m_VideoEncoder->Start();
	} catch (...) {
		throw;
	}
}

void Plugin::Interface::H264Interface::close()
{
	
}

bool Plugin::Interface::H264Interface::encode_frame(struct encoder_frame* frame, struct encoder_packet* packet, bool* received_packet, bool is_key_frame)
{
	if (m_VideoEncoder==nullptr)
		return false;

	if (!frame || !packet || !received_packet)
		return false;

	bool retVal = false;

	try {
		retVal = m_VideoEncoder->Encode(frame, packet, received_packet, is_key_frame);
	} catch (std::exception e) {
		PLOG_ERROR("Exception during encoding: %s", e.what());
	} catch (...) {
		PLOG_ERROR("Unknown exception during encoding.");
	}

	return retVal;
}

bool Plugin::Interface::H264Interface::get_extra_data(uint8_t** extra_data, size_t* size)
{
	if (m_VideoEncoder==nullptr)
		return false;

	return m_VideoEncoder->GetExtraData(extra_data, size);
}

void Plugin::Interface::H264Interface::init(const AmfEncoderConfig& config)
{
	auto api = API::GetAPI("");
	union {
		int64_t  v;
		uint32_t id[2];
	} adapterid  = {0, };
	auto adapter = api->GetAdapterById(adapterid.id[0], adapterid.id[1]);

	ColorFormat colorFormat = ColorFormat::NV12;
	ColorSpace colorSpace = ColorSpace::BT601;
	bool _P_OPENCL_TRANSFER = false;
	bool _P_OPENCL_CONVERSION = false;
	bool _VIDEO_RANGE_FULL = false;
	bool _P_MULTITHREADING = false;
	//size_t _P_QUEUESIZE = 8;
	size_t _P_QUEUESIZE = 0;	// for remove delay from input to output

	m_VideoEncoder = std::make_unique<EncoderH264>(
		api, adapter, _P_OPENCL_TRANSFER, _P_OPENCL_CONVERSION,
		colorFormat, colorSpace, _VIDEO_RANGE_FULL, _P_MULTITHREADING, _P_QUEUESIZE);

	//m_VideoEncoder->SetUsage(Plugin::AMD::Usage::Transcoding);
	m_VideoEncoder->SetUsage(Plugin::AMD::Usage::UltraLowLatency);

	switch (config.quality_preset)
	{
		case AmfQualityPreset::Speed :
			m_VideoEncoder->SetQualityPreset(QualityPreset::Speed);
			break;
		case AmfQualityPreset::Balanced :
			m_VideoEncoder->SetQualityPreset(QualityPreset::Balanced);
			break;
		case AmfQualityPreset::Quality :
			m_VideoEncoder->SetQualityPreset(QualityPreset::Quality);
			break;
		default :
			m_VideoEncoder->SetQualityPreset(QualityPreset::Speed);
	}
	
	/// Frame
	m_VideoEncoder->SetResolution(std::make_pair(config.width, config.height));
	m_VideoEncoder->SetFrameRate(std::make_pair(config.fps_num, config.fps_den));

	/// Profile & Level
	m_VideoEncoder->SetProfile(Profile::High);
	m_VideoEncoder->SetProfileLevel(ProfileLevel::Automatic, std::make_pair(config.width, config.height), std::make_pair(config.fps_num, config.fps_den));

	m_VideoEncoder->SetBFramePattern(0);

	try {
		m_VideoEncoder->SetCodingType(CodingType::Automatic);
	} catch (...) {
	}
	try {
		m_VideoEncoder->SetMaximumReferenceFrames(4);
	} catch (...) {
	}

	m_VideoEncoder->SetQPMinimum(18);
	m_VideoEncoder->SetQPMaximum(51);
	m_VideoEncoder->SetTargetBitrate(min(config.encoder_bitrate, config.target_bitrate));
	m_VideoEncoder->SetPeakBitrate(min(config.encoder_bitrate, config.max_bitrate));

	try {
		m_VideoEncoder->SetPrePassMode(PrePassMode::Disabled);
	} catch (...) {
	}
	try {
		m_VideoEncoder->SetVarianceBasedAdaptiveQuantizationEnabled(false);
	} catch (...) {
	}
	
	if (config.rate_control==AmfRateControl::CBR) {
		m_VideoEncoder->SetPeakBitrate(config.target_bitrate);
		m_VideoEncoder->SetFillerDataEnabled(true);
	} else {
		m_VideoEncoder->SetFillerDataEnabled(false);
	}

	m_VideoEncoder->SetFrameSkippingEnabled(false);
	m_VideoEncoder->SetEnforceHRDEnabled(true);

	/*
	 - VBV(Video Buffer Verifier)
		- 비디오 버퍼의 크기가 제한되어 있는 하드웨어 재생을 위한 동영상을 만들 경우
		- 스트리밍처럼 전송용 동영상을 만드는 경우에 주로 사용
		- 각종 휴대기기나 하드웨어 재생기에서 재생할 목적으로 인코딩하는 경우에는 VBV를 사용하는 편이 좋습니다.

	 - VBV Initial Buffer
		- VBV의 버퍼에 데이터가 어느정도 채워졌을 때 재생을 시작할지 결정하는 옵션입니다.
		0 ~ 1 사이의 값이 사용 가능하고 기본값은 0.9입니다. VBV가 사용되지 않으면 이 옵션도 무시됩니다.
  
	*/

	m_VideoEncoder->SetVBVBufferInitialFullness(1.00000000);
	m_VideoEncoder->SetVBVBufferStrictness(0.50000000000000000);
	m_VideoEncoder->SetVBVBufferSize(config.max_bitrate);

	double_t framerate = (double_t)config.fps_num / (double_t)config.fps_den;
	//m_VideoEncoder->SetIDRPeriod(5*config.fps_num);	// 5 seconds
	m_VideoEncoder->SetIDRPeriod(INFINITE);

	m_VideoEncoder->SetIFramePeriod(0);
	m_VideoEncoder->SetPFramePeriod(0);
	m_VideoEncoder->SetBFramePeriod(0);
	m_VideoEncoder->SetFrameSkippingPeriod(0);

	m_VideoEncoder->SetFrameSkippingBehaviour(false);
	m_VideoEncoder->SetDeblockingFilterEnabled(true);
	m_VideoEncoder->SetMotionEstimationHalfPixelEnabled(true);
	m_VideoEncoder->SetMotionEstimationQuarterPixelEnabled(true);

	if (config.rate_control==AmfRateControl::CBR) {
		m_VideoEncoder->SetRateControlMethod(RateControlMethod::ConstantBitrate);
		m_VideoEncoder->SetFillerDataEnabled(true);
	} else if (config.rate_control==AmfRateControl::VBR) {
		m_VideoEncoder->SetRateControlMethod(RateControlMethod::PeakConstrainedVariableBitrate);
	} else if (config.rate_control==AmfRateControl::VBR_LAT) {
		m_VideoEncoder->SetRateControlMethod(RateControlMethod::LatencyConstrainedVariableBitrate);
	} else if (config.rate_control==AmfRateControl::CQP) {
		m_VideoEncoder->SetRateControlMethod(RateControlMethod::ConstantQP);
	}
}

void Plugin::Interface::H264Interface::update(const AmfEncoderConfig& config)
{
	m_VideoEncoder->SetTargetBitrate(min(config.encoder_bitrate, config.target_bitrate));
	m_VideoEncoder->SetPeakBitrate(min(config.encoder_bitrate, config.max_bitrate));

	if (config.rate_control==AmfRateControl::CBR) {
		m_VideoEncoder->SetPeakBitrate(config.target_bitrate);
		m_VideoEncoder->SetFillerDataEnabled(true);
	} else {
		m_VideoEncoder->SetFillerDataEnabled(false);
	}

	m_VideoEncoder->SetVBVBufferSize(config.max_bitrate);

	m_VideoEncoder->SetDebug(true);
	if (m_VideoEncoder->IsStarted()) {
		m_VideoEncoder->LogProperties();
	}
}