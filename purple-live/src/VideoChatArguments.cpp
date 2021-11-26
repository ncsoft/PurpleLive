#include "VideoChatArguments.h"
#include "AppUtils.h"
#include <sstream>

static inline bool arg_is(const char *arg, const char *key)
{
	return (key && strcmpi(arg, key) == 0);
}

std::string VideoChatArguments::GetProgramPath(std::string commandLine)
{
	const char delimiter = ' ';
	std::string token{""};
	std::stringstream ss(commandLine);

	while (std::getline(ss, token, delimiter)) {
		break;
	}

	return token;
}

std::string VideoChatArguments::GetArguments(std::string commandLine, std::string path)
{
	return commandLine.substr(path.length()+1);
}

bool VideoChatArguments::IsForceExcute(int &argc, char **argv)
{
	for (int i = 1; i < argc; i++)
	{
		if (arg_is(argv[i], "--force-execute"))
		{
			return StringUtils::IsEqualNoCase("true", argv[i+1]);
		}
	}
	return false;
}

void VideoChatArguments::Parse(QStringList& args)
{
	if (args.size() >= 2) m_standAlone = true;

	for (int i = 1; i < args.size(); i++)
	{
		if (args[i] == "--service-network")
		{
			if (args[i+1] == "op")
			{
				m_serviceNetwork = eServiceNetwork::eSN_OP;
			}
			else if (args[i+1] == "sandbox")
			{
				m_serviceNetwork = eServiceNetwork::eSN_SB;
			}
			else if (args[i+1] == "rc")
			{
				m_serviceNetwork = eServiceNetwork::eSN_RC;
			}
			else if (args[i+1] == "live")
			{
				m_serviceNetwork = eServiceNetwork::eSN_LIVE;
			}
			else
			{
				assert(0);
			}
		}
		else if (args[i] == "--app-group-code")
		{
			m_gameCode = args[i+1].toStdString();
		}
		else if (args[i] == "--app-character-code")
		{
			m_characterCode = args[i+1].toStdString();
		}
		else if (args[i] == "--token")
		{
			m_token = args[i+1].toStdString();
		}
		else if (args[i] == "--device-id")
		{
			m_deviceId = args[i+1].toStdString();
		}
		else if (args[i] == "--authn-token")
		{
			m_authn_token = args[i+1].toStdString();
		}
		else if (args[i] == "--game-hwnd")
		{
			m_gameHwnd = args[i+1].toStdString();
		}
		else if (args[i] == "--game-id")
		{
			m_gameId = args[i+1].toStdString();
		}
		else if (args[i] == "--agent-port")
		{
			m_agentPort = args[i+1].toStdString();
		}
		else if (args[i] == "--instance-name")
		{
			m_instanceName = args[i+1].toStdString();
		}
		else if (args[i] == "--character-name")
		{
			m_characterName = StringUtils::GetStringFromUtf8(args[i+1].toStdString());
		}
		else if (args[i] == "--force-execute")
		{
			m_forceExcute = StringUtils::IsEqualNoCase("true", args[i+1].toStdString());
		}
		else if (args[i] == "--room-id")
		{
			m_roomId = args[i+1].toStdString();
		}
		else if (args[i] == "--group-id")
		{
			m_groupId = args[i+1].toStdString();
		}
		else if (args[i] == "--channel-id")
		{
			m_channelId = args[i+1].toStdString();
		}
		else if (args[i] == "--language")
		{
			m_language = args[i+1].toStdString();
		}
	}
}