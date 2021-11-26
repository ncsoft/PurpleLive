#pragma once

#include "MediaRoomBase.h"

class CameraCaptureMediaRoom : public MediaRoomBase
{
public:
	virtual bool	startMediaRoom() override;
	virtual void	stopMediaRoom() override;

	virtual void	Init(const string& url, const string& authkey, VideoEncodingType encodeType, int framerate) override;

protected:
	virtual void	Destroy() override;
};
