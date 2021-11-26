#include "MaskingManager.h"
#include <json.h>
#include <iostream>
#include <fstream>
#include "VideoChatApp.h"
#include "AppUtils.h"
#include "AppString.h"
#include <atlconv.h>
#include "easylogging++.h"

MaskingManager::MaskingManager()
{
}

MaskingManager::~MaskingManager()
{
	m_vtrMask.clear();
	elogd("[MaskingManager::~MaskingManager] Singleton");
}

void MaskingManager::Setup(const string gamecode)
{
	m_gameCode = gamecode;
	
	QString pathFilename = QApplication::applicationDirPath() + "/data/mask/" + QString::fromStdString(m_gameCode) + "/" + 
		QString::fromStdString(m_gameCode) + "_" + App()->GetLocalLanguage() + ".json";
	if (!AppUtils::fileExists(pathFilename))
	{
		// TODO: error
		return;
	}

	_LoadMasks(pathFilename.toLocal8Bit().toStdString());
}

void MaskingManager::RenderMasks()
{
	VTR_MASKINFO_ITR itr;
	for (itr = m_vtrMask.begin(); itr != m_vtrMask.end(); itr++)
	{
		const MaskInfo& mi = (*itr);
		if (mi.show == true)
		{
			uint32_t color = 0xffff00ff;
			MainWindow* pMainWnd = static_cast<MainWindow*>(App()->GetMainWindow());
			QSize size = pMainWnd->GetPreviewWidget()->GetSourceSize();

			SIZE gameSize = { size.width(), size.height() };
			if (mi.sprite)
			{
				mi.sprite->Draw(gameSize, mi.frect, DirectX::Colors::White);
			}
			else
			{
				NCCapture::Instance()->DrawRectInGame(gameSize, mi.frect, color);
			}
			
		}
	}
}

void MaskingManager::GetMaskNameList(vector<wstring>& maskNameList)
{
	maskNameList.clear();

	VTR_MASKINFO_ITR itr;
	for (itr = m_vtrMask.begin(); itr != m_vtrMask.end(); itr++)
	{
		wstring name = (*itr).name;
		maskNameList.push_back(name);
	}
	
}
/*
void MaskingManager::SetVisible(const wstring& name, bool visible)
{
	VTR_MASKINFO_ITR itr;
	for (itr = m_vtrMask.begin(); itr != m_vtrMask.end(); itr++)
	{
		wstring _name = (*itr).name;
		if (_name == name)
		{
			(*itr).show = visible;
		}
	}
}
*/
void MaskingManager::SetVisible(const int index, bool visible)
{
	if (index >= m_vtrMask.size())
	{
		LOG(ERROR) << format_string("[MaskingManager::SetVisible] out fo bound %d", index);
		return;
	}
	m_vtrMask[index].show = visible;
}

bool MaskingManager::_LoadMasks(const string& filename)
{
	m_vtrMask.clear();

	Json::Reader reader;
	Json::Value root;
	
	ifstream json(filename.c_str(), ifstream::binary);

	reader.parse(json, root);

	const Json::Value& jResolution = root["resolution"];
	if (jResolution.empty())
	{
		return false;
	}

	float width = (float)root["resolution"]["width"].asInt();
	float height = (float)root["resolution"]["height"].asInt();

	const Json::Value& jMasks = root["masks"];

	for (const Json::Value& mask : jMasks)
	{
		MaskInfo mi{};
		string name		= mask["name"].asString();
		USES_CONVERSION;
		wstring wname = A2W_CP(name.c_str(), CP_UTF8);

		string file = mask["file"].asString();

		mi.name		= wname;
		mi.show		= false;

		RECT rc;
		rc.left		= mask["left"].asInt();
		rc.top		= mask["top"].asInt();
		rc.right	= mask["right"].asInt();
		rc.bottom	= mask["bottom"].asInt();
		
		std::string pathFilename = QApplication::applicationDirPath().toStdString() + "/data/mask/" + m_gameCode + "/" + file;
		wstring wfile = A2W_CP(pathFilename.c_str(), CP_UTF8);
		
		if (mi.sprite->Create(wfile.c_str()) == false)
		{
			mi.sprite.release();
			LOG(ERROR) << format_string("[MaskingManager::load_mask] failed to load %s file", file.c_str());
		}

		// convert to scale coord'
		mi.frect.left	= rc.left / width;
		mi.frect.top	= rc.top / height;
		mi.frect.right	= rc.right / width;
		mi.frect.bottom	= rc.bottom / height;

		m_vtrMask.push_back(std::move(mi));
	}

	return true;
}
