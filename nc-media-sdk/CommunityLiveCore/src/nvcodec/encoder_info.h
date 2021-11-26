#ifndef _ENCODER_INFO_H
#define _ENCODER_INFO_H

#include <cstdint>
#include <string>
#include <vector>
#include <dxgi.h>
#include <d3d11.h>

enum class NvencQualityPreset : uint8_t {
	Speed,
	Balanced,
	Quality,
};

enum class NvencRateControl: uint8_t {
	CONSTQP                = 0x0,       /**< Constant QP mode */
    VBR                    = 0x1,       /**< Variable bitrate mode */
    CBR                    = 0x2,       /**< Constant bitrate mode */
    CBR_LOWDELAY_HQ        = 0x8,       /**< low-delay CBR, high quality */
    CBR_HQ                 = 0x10,      /**< CBR, high quality (slower) */
    VBR_HQ                 = 0x20       /**< VBR, high quality (slower) */
};

struct encoder_config
{
	uint32_t width;
	uint32_t height;
	uint32_t framerate;
	uint32_t bitrate;
	uint32_t gop;
	std::string codec;  // "h264" 
	DXGI_FORMAT format; // DXGI_FORMAT_NV12 DXGI_FORMAT_B8G8R8A8_UNORM

	NvencRateControl rate_control = NvencRateControl::VBR;
	NvencQualityPreset quality_preset = NvencQualityPreset::Speed;
};

struct encoder_info
{
	bool (*is_supported)(void);
	void* (*create)(void);
	void (*destroy)(void **encoder_data);
	bool (*init)(void *encoder_data, void *encoder_config);
	int (*encode_texture)(void *nvenc_data, ID3D11Texture2D *texture, std::vector<std::vector<uint8_t>>& out_packets);
	int (*encode_handle)(void *nvenc_data, HANDLE handle, int lock_key, int unlock_key, std::vector<std::vector<uint8_t>>& out_packets);
	int(*set_bitrate)(void *nvenc_data, uint32_t bitrate_bps);
	int(*set_framerate)(void *nvenc_data, uint32_t framerate);
	int(*request_idr)(void *nvenc_data);
	int(*get_sequence_params)(void *nvenc_data, uint8_t* buf, uint32_t maxBufSize);
	ID3D11Device* (*get_device)(void *encoder_data);
	ID3D11Texture2D* (*get_texture)(void *encoder_data);
	ID3D11DeviceContext* (*get_context)(void *encoder_data);
};

#endif