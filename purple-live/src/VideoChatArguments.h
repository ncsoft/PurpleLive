#pragma once

#include "lime-defines.h"
#include <string>

struct VideoChatArguments
{
	eServiceNetwork			m_serviceNetwork;
	std::string				m_characterName;
	std::string				m_gameCode;
	std::string				m_characterCode;
	std::string				m_roomId;
	std::string				m_token;
	std::string				m_deviceId = "e0000000000000000000000000000000";
	std::string				m_authn_token; //for apiGate(MAP)
	std::string				m_gameHwnd;
	std::string				m_gameId="";
	std::string				m_version;
	std::string				m_agentPort;
	std::string				m_instanceName;
	std::string				m_groupId;
	std::string				m_channelId;
	std::string				m_language;
	bool					m_forceExcute = false;
	bool					m_standAlone = false;

	static std::string GetProgramPath(std::string commandLine);
	static std::string GetArguments(std::string commandLine, std::string path);
	static bool IsForceExcute(int &argc, char **argv);
	void Parse(QStringList& args);
};