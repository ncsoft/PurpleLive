#pragma once

#include "modules/include/module_common_types.h"
#include "common_video/h264/h264_common.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "avc_util.hpp"
#include "../src/util.h"
#include "easylogging++.h"

namespace webrtc {

class VideoEncoderUtil
{
public:
	static bool IsKeyFrame(const std::vector<uint8_t>& encoded_output_buffer)
	{
		RTPFragmentationHeader frag_header;

		auto encoded_buffer_size = encoded_output_buffer.size();
		std::vector<H264::NaluIndex> NALUidx;
		auto p_nal = encoded_output_buffer.data();
		NALUidx = H264::FindNaluIndices(p_nal, encoded_buffer_size);
		size_t i_nal = NALUidx.size();
		if (i_nal == 0)
		{
			return false;
		}

		if (i_nal == 1)
		{
			NALUidx[0].payload_size = encoded_buffer_size - NALUidx[0].payload_start_offset;
		}
		else for (size_t i = 0; i < i_nal; i++)
		{
			NALUidx[i].payload_size = i + 1 >= i_nal ? encoded_buffer_size - NALUidx[i].payload_start_offset : NALUidx[i + 1].start_offset - NALUidx[i].payload_start_offset;
		}

		for (size_t nal_index = 0; nal_index < i_nal; nal_index++)
		{
			uint8_t naluType = H264::ParseNaluType(p_nal[NALUidx[nal_index].payload_start_offset]);
			if (naluType == (uint8_t)AVC_NAL_TYPE::AVC_NAL_IDR) {
				return true;
			}
		}

		return false;
	}

	static bool MakeRtpFragmentationHeader(const std::vector<uint8_t>& encoded_output_buffer, RTPFragmentationHeader& frag_header, bool IsKeyFrame) 
	{
		auto encoded_buffer_size = encoded_output_buffer.size();
        std::vector<H264::NaluIndex> NALUidx;
        auto p_nal = encoded_output_buffer.data();
        NALUidx = H264::FindNaluIndices(p_nal, encoded_buffer_size);
		size_t i_nal = NALUidx.size();
        if (i_nal == 0)
        {
			return false;
        }

        if (i_nal == 1)
        {
            NALUidx[0].payload_size = encoded_buffer_size - NALUidx[0].payload_start_offset;
        }
        else for (size_t i = 0; i < i_nal; i++)
        {
            NALUidx[i].payload_size = i + 1 >= i_nal ? encoded_buffer_size - NALUidx[i].payload_start_offset : NALUidx[i + 1].start_offset - NALUidx[i].payload_start_offset;
        }

		frag_header.VerifyAndAllocateFragmentationHeader(i_nal);

        uint32_t totalNaluIndex = 0;
        for (size_t nal_index = 0; nal_index < i_nal; nal_index++)
        {
			uint8_t naluType = H264::ParseNaluType(p_nal[NALUidx[nal_index].payload_start_offset]);
            const size_t currentNaluSize = NALUidx[nal_index].payload_size;
            frag_header.fragmentationOffset[totalNaluIndex] = NALUidx[nal_index].payload_start_offset;
            frag_header.fragmentationLength[totalNaluIndex] = currentNaluSize;
            //frag_header.fragmentationPlType[totalNaluIndex] = naluType;
            //frag_header.fragmentationTimeDiff[totalNaluIndex] = 0;
            totalNaluIndex++;

#if 0 // NAL type log
			if (IsKeyFrame) 
			{
				switch(naluType)
				{
					case (uint8_t)AVC_NAL_TYPE::AVC_NAL_IDR:
						ELOGD << format_string("[VideoEncoderUtil::MakeRtpFragmentationHeader] maked KeyFrame - naluType(IDR)(%d)", naluType); break;
					case (uint8_t)AVC_NAL_TYPE::AVC_NAL_SEI:
						ELOGD << format_string("[VideoEncoderUtil::MakeRtpFragmentationHeader] maked KeyFrame - naluType(SEI)(%d)", naluType); break;
					case (uint8_t)AVC_NAL_TYPE::AVC_NAL_SPS:
						ELOGD << format_string("[VideoEncoderUtil::MakeRtpFragmentationHeader] maked KeyFrame - naluType(SPS)(%d)", naluType); break;
					case (uint8_t)AVC_NAL_TYPE::AVC_NAL_PPS:
						ELOGD << format_string("[VideoEncoderUtil::MakeRtpFragmentationHeader] maked KeyFrame - naluType(PPS)(%d)", naluType); break;
					default:
						ELOGD << format_string("[VideoEncoderUtil::MakeRtpFragmentationHeader] maked KeyFrame - naluType(...)(%d)", naluType);
				}
			}
#endif
        }

		return true;
	}
};

}  // namespace webrtc