#pragma once
#include <NCCaptureSDK.h>
#include <string>
#include <vector>

using namespace std;

struct MaskInfo
{
	wstring	name;
	bool	show;
	FRECT	frect;
	unique_ptr<GameSprite>	sprite;

	MaskInfo() : sprite(make_unique<GameSprite>())
	{
		name = L"";
		show = true;
	}
};

typedef vector<MaskInfo>				VTR_MASKINFO;
typedef vector<MaskInfo>::iterator		VTR_MASKINFO_ITR;

class MaskingManager : public NCCaptureSDK::Singleton<MaskingManager>
{
public:
	MaskingManager();
	~MaskingManager();

	void	Setup(const string gamecode);
	void	RenderMasks();

	void	GetMaskNameList(vector<wstring>& maskNameList);
//	void	SetVisible(const wstring& name, bool visible); deprecate
	void	SetVisible(const int index, bool visible);

protected:
	bool	_LoadMasks(const string& filename);
	VTR_MASKINFO	m_vtrMask;
	string			m_gameCode;
};

