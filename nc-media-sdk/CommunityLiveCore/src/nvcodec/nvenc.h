#ifndef _NVIDIA_ENCODER_H
#define _NVIDIA_ENCODER_H

#include "encoder_info.h"

#ifndef NOT_USE_NVENC
// Video-Codec-SDK Version: 8.2
#include "NvEncoder/NvEncoderD3D11.h" 
#endif

extern struct encoder_info nvenc_info;

#endif
