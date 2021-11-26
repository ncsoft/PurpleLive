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

#pragma once
#include "amf-encoder-h264.hpp"
#include "plugin.hpp"

#define USE_FRAME_ENCODING_ONLY		1

enum class AmfQualityPreset : uint8_t {
	Speed,
	Balanced,
	Quality,
};

enum class AmfRateControl: uint8_t {
	CBR,
	VBR,
	VBR_LAT,
	CQP,
};

struct AmfEncoderConfig {
	int32_t width = 0;
	int32_t height = 0;
	int32_t target_bitrate = 2500 * 1000;
	int32_t max_bitrate = 2500 * 1000;
	int32_t encoder_bitrate = 2500 * 1000;
	int32_t fps_num = 30;
	int32_t fps_den = 1;
	AmfRateControl rate_control = AmfRateControl::VBR;
	AmfQualityPreset quality_preset = AmfQualityPreset::Speed;
};

namespace Plugin {
	namespace Interface {
		class H264Interface {
			public:
			H264Interface(const AmfEncoderConfig& config);
			~H264Interface();
			void open(const AmfEncoderConfig& config);
			void close();
			bool encode_frame(struct encoder_frame* frame, struct encoder_packet* packet, bool* received_packet, bool is_key_frame);
			bool get_extra_data(uint8_t** extra_data, size_t* size);
			void init(const AmfEncoderConfig& config);
			void update(const AmfEncoderConfig& config);
			
			private:
			std::unique_ptr<Plugin::AMD::EncoderH264> m_VideoEncoder;
		};
	} // namespace Interface
} // namespace Plugin
