#pragma once
#ifdef NCCAPTURESDK_EXPORTS
#define NCCAPTURESDK_API __declspec(dllexport)
#else
#define NCCAPTURESDK_API __declspec(dllimport)
#endif

struct FRECT
{
	float	left;
	float	top;
	float	right;
	float	bottom;

	FRECT()
	{}

	FRECT(float _left, float _top, float _right, float _bottom)
	{
		left = _left;
		top = _top;
		right = _right;
		bottom = _bottom;
	}
};

struct CapturedImage
{
	size_t      width;
	size_t      height;
	size_t      rowPitch;
	size_t      slicePitch;
	unsigned char*    pixels;	//R8G8B8A8 32bit 
};

typedef void(*DEVICE_CREATED_CALLBACK_FUNC)(void* user_data, bool recreate);

