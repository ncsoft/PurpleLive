#include "LimeLogin.hpp"
#include <curl.h>
#include <string>
#include <iostream>
#include <sstream>
#include <memory>
#include <cstdio>
#include <map>
#include "AppConfig.h"
#include "AppUtils.h"
#include "LimeCommunicator.h"
#include "easylogging++.h"

// json utility function
string json_get_string_value(json_t* obj, string key)
{
	json_t* data = json_object_get(obj, key.c_str());
	if (json_is_string(data))
	{
		return json_string_value(data);
	}

	return "";
}

int json_get_integer_value(json_t* obj, string key)
{
	json_t* data = json_object_get(obj, key.c_str());
	if (json_is_integer(data))
	{
		return (int)json_integer_value(data);
	}

	return 0;
}

bool json_get_bool_value(json_t* obj, string key)
{
	json_t* data = json_object_get(obj, key.c_str());
	if (json_is_boolean(data))
	{
		return json_boolean_value(data);
	}

	return false;
}

// string util function ----------------------------------------------------------------------
template<typename... Args>
std::string string_format(const std::string &format, Args... args)
{
	size_t size = snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
	std::unique_ptr<char[]> buf(new char[size]);
	snprintf(buf.get(), size, format.c_str(), args...);
	return std::string(buf.get(),
			   buf.get() + size -
				   1); // We don't want the '\0' inside
}
// -----------------------------------------------------------------------------------------

static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
			    void *userp)
{
	((std::string *)userp)->append((char *)contents, size * nmemb);
	return size * nmemb;
}

void initHttpHeader(struct curl_slist** ppHeaderList, const char* token, const char* appVersion)
{
	*ppHeaderList = curl_slist_append(*ppHeaderList, string_format("Lime-Trace-Id: %s", generate_lime_trace_id().c_str()).c_str());
	*ppHeaderList = curl_slist_append(*ppHeaderList, string_format("Authorization: %s", token).c_str());
	*ppHeaderList = curl_slist_append(*ppHeaderList, "Lime-OS-Version: Windows");
	*ppHeaderList = curl_slist_append(*ppHeaderList, "Lime-Sdk-Version: MLiveSDK.0.1");
	*ppHeaderList = curl_slist_append(*ppHeaderList, "Content-Type: application/json;charset=UTF-8");
	*ppHeaderList = curl_slist_append(*ppHeaderList, "Lime-API-Version: 100");
	*ppHeaderList = curl_slist_append(*ppHeaderList, AppUtils::format("Lime-App-Version: %s v%s", AppConfig::APP_NAME, appVersion).c_str());
	*ppHeaderList = curl_slist_append(*ppHeaderList, "Lime-Device-Name: desktop");

	if (*ppHeaderList)
	{
		curl_slist* p = *ppHeaderList;
		while (p)
		{
			LOG(DEBUG) << p->data;
			p = p->next;
		}
	}
}

bool LoginWithJWToken(const char* gameUserID, const char* token, const char* deviceId, StompInfo& info, eServiceNetwork sn, const char* appVersion, const char* languageCode)
{
	CURL *curl;
	CURLcode res;

	std::string strTargetURL;
	std::string strResourceJSON;
	std::string readBuffer;
	std::string readHeader;

	strTargetURL = get_lime_api_domain(sn) + "/stream/login/loginWithPurple";
	LOG(DEBUG) << "------------Send------------";
	LOG(DEBUG) << strTargetURL;

	struct curl_slist *headerlist = nullptr;
	initHttpHeader(&headerlist, token, appVersion);

	json_t *root = json_object();
	json_object_set_new(root, "gameUserId", json_string(gameUserID));
	json_object_set_new(root, "clientType", json_string("PC"));
	json_object_set_new(root, "deviceId", json_string(deviceId));
	json_object_set_new(root, "locale", json_string(languageCode));
	strResourceJSON = json_dumps(root, 0);
	LOG(DEBUG) << strResourceJSON;

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, strTargetURL.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);

		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS,
			strResourceJSON.c_str());

		// 결과 기록
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&readBuffer);
		curl_easy_setopt(curl, CURLOPT_WRITEHEADER, (void *)&readHeader);

		res = curl_easy_perform(curl);

		if (res != CURLE_OK)
		{
			eloge("curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));
			return false;
		}
		long http_code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
		LOG(DEBUG) << "------------Receive------------";
		LOG(DEBUG) << readHeader;
		LOG(DEBUG) << StringUtils::utf8_to_string(readBuffer);

		curl_easy_cleanup(curl);
		curl_slist_free_all(headerlist);

		if (http_code != 200)
		{
			LOG(ERROR) << "<------------Lime Error------------";
			LOG(ERROR) << strTargetURL;
			LOG(ERROR) << strResourceJSON;
			LOG(ERROR) << readHeader;
			LOG(ERROR) << StringUtils::utf8_to_string(readBuffer.c_str());
			LOG(ERROR) << "------------Lime Error------------>";
			return false;
		}

		json_error_t error;
		json_t *root = json_loads(readBuffer.c_str(), 0, &error);

		if (!root)
		{
			eloge("error: on line %d: %s\n", error.line, error.text);
			return false;
		}

		// parse header -------------------------------------------
		std::map<std::string, std::string> m;

		std::istringstream resp(readHeader);
		std::string header;
		std::string::size_type index;
		while (std::getline(resp, header) && header != "\r")
		{
			index = header.find(':', 0);
			if (index != std::string::npos)
			{
				m.insert(std::make_pair(StringUtils::trim(header.substr(0, index)), StringUtils::trim(header.substr(index + 1))));
			}
		}

		std::map<std::string, std::string>::iterator itr = m.find("Authorization");
		if (itr == m.end())
		{
			json_decref(root);
			return false;
		}
		info.jwt = itr->second;

		// parse body ---------------------------------------------
		// count
		json_t* subscriptionInfo = json_object_get(root, "subscriptionInfo");
		if (!json_is_object(subscriptionInfo))
		{
			json_decref(root);
			return false;
		}
		else
		{
			json_t* topicName = json_object_get(subscriptionInfo, "topicName");
			if (json_is_string(topicName))
			{
				info.topicName = json_string_value(topicName);
			}

			json_t* subscribeUrl = json_object_get(subscriptionInfo, "subscribeUrl");
			if (json_is_string(subscribeUrl))
			{
				info.subscribeURL = json_string_value(subscribeUrl);
			}

			json_t* login = json_object_get(subscriptionInfo, "login");
			if (json_is_string(login))
			{
				info.login = json_string_value(login);
			}

			json_t* passcode = json_object_get(subscriptionInfo, "passcode");
			if (json_is_string(passcode))
			{
				info.passcode = json_string_value(passcode);
			}

			json_t* serverAppDest = json_object_get(subscriptionInfo, "serverAppDest");
			if (json_is_string(serverAppDest))
			{
				info.serverAppDest = json_string_value(serverAppDest);
			}
		}
		// parse --------------------------------------------------
		return true;
	}
	return false;
}

bool GetSubscriptionInfo(const char* token, StompInfo& info, eServiceNetwork sn, const char* appVersion, const char* languageCode)
{
	CURL *curl;
	CURLcode res;

	std::string strTargetURL;
	std::string strResourceJSON;
	std::string readBuffer;
	std::string readHeader;

	strTargetURL = get_lime_api_domain(sn) + "/stream/user/getSubscriptionInfo";
	LOG(DEBUG) << "------------Send------------";
	LOG(DEBUG) << strTargetURL;

	struct curl_slist *headerlist = nullptr;
	initHttpHeader(&headerlist, token, appVersion);

	json_t *root = json_object();
	json_object_set_new(root, "locale", json_string(languageCode));
	strResourceJSON = json_dumps(root, 0);
	LOG(DEBUG) << strResourceJSON;

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, strTargetURL.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);

		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS,
			strResourceJSON.c_str());

		// 결과 기록
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&readBuffer);
		curl_easy_setopt(curl, CURLOPT_WRITEHEADER, (void *)&readHeader);

		res = curl_easy_perform(curl);

		if (res != CURLE_OK)
		{
			eloge("curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));
			return false;
		}
		long http_code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
		LOG(DEBUG) << "------------Receive------------";
		LOG(DEBUG) << readHeader;
		LOG(DEBUG) << StringUtils::utf8_to_string(readBuffer);

		curl_easy_cleanup(curl);
		curl_slist_free_all(headerlist);

		if (http_code != 200)
		{
			LOG(ERROR) << "<------------Lime Error------------";
			LOG(ERROR) << strTargetURL;
			LOG(ERROR) << strResourceJSON;
			LOG(ERROR) << readHeader;
			LOG(ERROR) << StringUtils::utf8_to_string(readBuffer.c_str());
			LOG(ERROR) << "------------Lime Error------------>";
			return false;
		}

		json_error_t error;
		json_t *root = json_loads(readBuffer.c_str(), 0, &error);

		if (!root)
		{
			eloge("error: on line %d: %s\n", error.line, error.text);
			return false;
		}

		// parse header -------------------------------------------
		std::map<std::string, std::string> m;

		std::istringstream resp(readHeader);
		std::string header;
		std::string::size_type index;
		while (std::getline(resp, header) && header != "\r")
		{
			index = header.find(':', 0);
			if (index != std::string::npos)
			{
				m.insert(std::make_pair(StringUtils::trim(header.substr(0, index)), StringUtils::trim(header.substr(index + 1))));
			}
		}

		// parse body ---------------------------------------------
		// count
		json_t* subscriptionInfo = json_object_get(root, "subscriptionInfo");
		if (!json_is_object(subscriptionInfo))
		{
			json_decref(root);
			return false;
		}
		else
		{
			json_t* topicName = json_object_get(subscriptionInfo, "topicName");
			if (json_is_string(topicName))
			{
				info.topicName = json_string_value(topicName);
			}

			json_t* subscribeUrl = json_object_get(subscriptionInfo, "subscribeUrl");
			if (json_is_string(subscribeUrl))
			{
				info.subscribeURL = json_string_value(subscribeUrl);
			}

			json_t* login = json_object_get(subscriptionInfo, "login");
			if (json_is_string(login))
			{
				info.login = json_string_value(login);
			}

			json_t* passcode = json_object_get(subscriptionInfo, "passcode");
			if (json_is_string(passcode))
			{
				info.passcode = json_string_value(passcode);
			}

			json_t* serverAppDest = json_object_get(subscriptionInfo, "serverAppDest");
			if (json_is_string(serverAppDest))
			{
				info.serverAppDest = json_string_value(serverAppDest);
			}
		}
		// parse --------------------------------------------------
		return true;
	}
	return false;
}

eGetCharacterErrorType GetCharacterList(const char* gameCode, const char* token, vector<GameUserInfo> &list, eServiceNetwork sn, const char* appVersion, const char* languageCode)
{
	CURL *curl;
	CURLcode res;

	std::string strTargetURL;
	std::string strResourceJSON;
	std::string readBuffer;
	std::string readHeader;

	strTargetURL = get_lime_api_domain(sn) + "/stream/purple/getCharacterList";
	LOG(DEBUG) << "------------Send------------";
	LOG(DEBUG) << strTargetURL;

	struct curl_slist *headerlist = nullptr;
	initHttpHeader(&headerlist, token, appVersion);

	json_t *root = json_object();
	json_object_set_new(root, "gameCode", json_string(gameCode));
	json_object_set_new(root, "locale", json_string(languageCode));
	strResourceJSON = json_dumps(root, 0);
	LOG(DEBUG) << strResourceJSON;

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, strTargetURL.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);

		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strResourceJSON.c_str());

		// 결과 기록
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&readBuffer);
		curl_easy_setopt(curl, CURLOPT_WRITEHEADER, (void *)&readHeader);

		res = curl_easy_perform(curl);

		if (res != CURLE_OK)
		{
			eloge("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			return eGetCharacterErrorType::eFailFindCharacter;
		}
		long http_code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

		LOG(DEBUG) << "------------Receive------------";
		LOG(DEBUG) << readHeader;
		LOG(DEBUG) << StringUtils::utf8_to_string(readBuffer);

		curl_easy_cleanup(curl);
		curl_slist_free_all(headerlist);

		if (http_code != 200)
		{
			LOG(ERROR) << "<------------Lime Error------------";
			LOG(ERROR) << strTargetURL;
			LOG(ERROR) << strResourceJSON;
			LOG(ERROR) << readHeader;
			LOG(ERROR) << StringUtils::utf8_to_string(readBuffer.c_str());
			LOG(ERROR) << "------------Lime Error------------>";
			return eGetCharacterErrorType::eFailFindCharacter;
		}

		json_error_t error;
		json_t *root = json_loads(readBuffer.c_str(), 0, &error);

		if (!root)
		{
			eloge("error: on line %d: %s\n", error.line, error.text);
			return eGetCharacterErrorType::eFailFindCharacter;
		}

		// parse --------------------------------------------------
		// count
		json_t* count = json_object_get(root, "count");
		if (!json_is_integer(count))
		{
			json_decref(root);
			return eGetCharacterErrorType::eFailFindCharacter;
		}
		// int nCount = (int)json_integer_value(count);

		// characterList
		json_t* characterList = json_object_get(root, "characterList");
		if (!json_is_array(characterList))
		{
			eloge("error: root is not an array\n");
			json_decref(root);
			return eGetCharacterErrorType::eFailFindCharacter;
		}

		for (int i = 0; i<json_array_size(characterList); i++)
		{
			json_t* character = json_array_get(characterList, i);
			if (!json_is_object(character))
			{
				eloge("error: character %d is not an object\n", i + 1);
				json_decref(root);
				return eGetCharacterErrorType::eFailFindCharacter;
			}
			else
			{
				struct GameUserInfo info;
				json_t* canStreaming = json_object_get(character, "canStreaming");
				if (!json_is_boolean(canStreaming))
				{
					eloge("error: canStreaming field does not exist");
					json_decref(root);
					return eGetCharacterErrorType::eFailFindCharacter;
				}
				info.canStreaming = json_boolean_value(canStreaming);

				json_t* canUseGuildType = json_object_get(character, "canUseGuildType");
				if (!json_is_boolean(canUseGuildType))
				{
					eloge("error: canUseGuildType field does not exist");
					json_decref(root);
					return eGetCharacterErrorType::eFailFindCharacter;
				}
				info.canUseGuildType = json_boolean_value(canUseGuildType);

				json_t* gameUserInfo = json_object_get(character, "gameUserInfo");
				if (!json_is_object(gameUserInfo))
				{
					eloge("error: gameUserInfo %d is not an object\n", i + 1);
					json_decref(root);
					return eGetCharacterErrorType::eFailFindCharacter;
				}
				else
				{
					json_t* gameCode = json_object_get(gameUserInfo, "gameCode");
					if (json_is_string(gameCode))
					{
						info.gameCode = json_string_value(gameCode);
					}

					json_t* profileImageUrlSmall = json_object_get(gameUserInfo, "profileImageUrlSmall");
					if (json_is_string(profileImageUrlSmall))
					{
						info.profileUrl = json_string_value(profileImageUrlSmall);
					}
					

					json_t* gameUserId = json_object_get(gameUserInfo, "gameUserId");
					if (json_is_string(gameUserId))
					{
						info.gameUserId = json_string_value(gameUserId);
					}

					json_t* characterName = json_object_get(gameUserInfo, "characterName");
					if (json_is_string(characterName))
					{
						info.characterName = json_string_value(characterName);
					}

					json_t* serverName = json_object_get(gameUserInfo, "serverName");
					if (json_is_string(serverName))
					{
						info.serverName = json_string_value(serverName);
					}

					if (info.canUseGuildType == false)
					{
						info.guildName = "";
					}
					else
					{
						json_t* guildName = json_object_get(gameUserInfo, "guildName");
						if (json_is_string(guildName))
						{
							info.guildName = json_string_value(guildName);
						}
					}
					
				}

				list.push_back(info);
			}
		}

		// if character list empty -> return false
		if(list.size() <= 0) return eGetCharacterErrorType::eNoStreamableCharacter;
			
		return eGetCharacterErrorType::eNoError;
	}
	return eGetCharacterErrorType::eFailFindCharacter;
}

bool CreateChatting(const char* token, vector<GameUserInfo> &list, eServiceNetwork sn, const char* appVersion, json_t** result, json_error_t& error, const char* params)
{
	CURL *curl;
	CURLcode res;

	std::string strTargetURL;
	std::string strResourceJSON{params};
	std::string readBuffer;
	std::string readHeader;

	strTargetURL = get_lime_api_domain(sn) + "/chat/createChatting";
	LOG(DEBUG) << "------------Send------------";
	LOG(DEBUG) << strTargetURL;

	struct curl_slist *headerlist = nullptr;
	initHttpHeader(&headerlist, token, appVersion);
	LOG(DEBUG) << strResourceJSON;

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, strTargetURL.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);

		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strResourceJSON.c_str());

		// 결과 기록
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&readBuffer);
		curl_easy_setopt(curl, CURLOPT_WRITEHEADER, (void *)&readHeader);

		res = curl_easy_perform(curl);

		if (res != CURLE_OK)
		{
			eloge("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			return false;
		}
		long http_code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

		LOG(DEBUG) << "------------Receive------------";
		LOG(DEBUG) << readHeader;
		LOG(DEBUG) << StringUtils::utf8_to_string(readBuffer.c_str());

		curl_easy_cleanup(curl);
		curl_slist_free_all(headerlist);

		if (http_code != 200)
		{
			LOG(ERROR) << "<------------Lime Error------------";
			LOG(ERROR) << strTargetURL;
			LOG(ERROR) << strResourceJSON;
			LOG(ERROR) << readHeader;
			LOG(ERROR) << StringUtils::utf8_to_string(readBuffer.c_str());
			LOG(ERROR) << "------------Lime Error------------>";
			return false;
		}

		*result = json_loads(readBuffer.c_str(), 0, &error);
		return true;
	}
	return false;
}

bool GetInvitationUserList(const char* token, vector<GameUserInfo> &list, eServiceNetwork sn, const char* appVersion, json_t** result, json_error_t& error, const char* gameUserId)
{
	CURL *curl;
	CURLcode res;

	std::string strTargetURL;
	std::string strResourceJSON;
	std::string readBuffer;
	std::string readHeader;

	strTargetURL = get_lime_api_domain(sn) + "/chat/getInvitationUserList";
	LOG(DEBUG) << "------------Send------------";
	LOG(DEBUG) << strTargetURL;

	struct curl_slist *headerlist = nullptr;
	initHttpHeader(&headerlist, token, appVersion);

	json_t *root = json_object();
	json_object_set_new(root, "gameUserId", json_string(gameUserId));
	json_object_set_new(root, "groupUserId", json_null());
	strResourceJSON = json_dumps(root, 0);
	LOG(DEBUG) << strResourceJSON;

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, strTargetURL.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);

		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strResourceJSON.c_str());

		// 결과 기록
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&readBuffer);
		curl_easy_setopt(curl, CURLOPT_WRITEHEADER, (void *)&readHeader);

		res = curl_easy_perform(curl);

		if (res != CURLE_OK)
		{
			eloge("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			return false;
		}
		long http_code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

		LOG(DEBUG) << "------------Receive------------";
		LOG(DEBUG) << readHeader;
		//LOG(DEBUG) << StringUtils::utf8_to_string(readBuffer.c_str());

		curl_easy_cleanup(curl);
		curl_slist_free_all(headerlist);

		if (http_code != 200) 
		{
			LOG(ERROR) << "<------------Lime Error------------";
			LOG(ERROR) << strTargetURL;
			LOG(ERROR) << strResourceJSON;
			LOG(ERROR) << readHeader;
			LOG(ERROR) << StringUtils::utf8_to_string(readBuffer.c_str());
			LOG(ERROR) << "------------Lime Error------------>";
			return false;
		}
		
		*result = json_loads(readBuffer.c_str(), 0, &error);
		return true;
	}
	return false;
}

bool GetGuildUserList(const char* token, vector<GameUserInfo> &list, eServiceNetwork sn, const char* appVersion, json_t** result, json_error_t& error)
{
	CURL *curl;
	CURLcode res;

	std::string strTargetURL;
	std::string strResourceJSON;
	std::string readBuffer;
	std::string readHeader;

	strTargetURL = get_lime_api_domain(sn) + "/stream/chat/getGuildUserList";
	LOG(DEBUG) << "------------Send------------";
	LOG(DEBUG) << strTargetURL;

	struct curl_slist *headerlist = nullptr;
	initHttpHeader(&headerlist, token, appVersion);

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, strTargetURL.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);

		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strResourceJSON.c_str());

		// 결과 기록
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&readBuffer);
		curl_easy_setopt(curl, CURLOPT_WRITEHEADER, (void *)&readHeader);

		res = curl_easy_perform(curl);

		if (res != CURLE_OK)
		{
			eloge("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			return false;
		}
		long http_code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

		LOG(DEBUG) << "------------Receive------------";
		LOG(DEBUG) << readHeader;
		//LOG(DEBUG) << StringUtils::utf8_to_string(readBuffer.c_str());

		curl_easy_cleanup(curl);
		curl_slist_free_all(headerlist);

		if (http_code != 200)
		{
			LOG(ERROR) << "<------------Lime Error------------";
			LOG(ERROR) << strTargetURL;
			LOG(ERROR) << strResourceJSON;
			LOG(ERROR) << readHeader;
			LOG(ERROR) << StringUtils::utf8_to_string(readBuffer.c_str());
			LOG(ERROR) << "------------Lime Error------------>";
			return false;
		}
		
		*result = json_loads(readBuffer.c_str(), 0, &error);
		return true;
	}
	return false;
}

bool GetShareTargetList(const char* token, vector<GameUserInfo> &list, eServiceNetwork sn, const char* appVersion, json_t** result, json_error_t& error)
{
	CURL *curl;
	CURLcode res;

	std::string strTargetURL;
	std::string strResourceJSON;
	std::string readBuffer;
	std::string readHeader;

	strTargetURL = get_lime_api_domain(sn) + "/stream/getShareTargetList";
	LOG(DEBUG) << "------------Send------------";
	LOG(DEBUG) << strTargetURL;

	struct curl_slist *headerlist = nullptr;
	initHttpHeader(&headerlist, token, appVersion);
	
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, strTargetURL.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);

		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strResourceJSON.c_str());

		// 결과 기록
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&readBuffer);
		curl_easy_setopt(curl, CURLOPT_WRITEHEADER, (void *)&readHeader);

		res = curl_easy_perform(curl);

		if (res != CURLE_OK)
		{
			eloge("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			return false;
		}
		long http_code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

		LOG(DEBUG) << "------------Receive------------";
		LOG(DEBUG) << readHeader;
		//LOG(DEBUG) << StringUtils::utf8_to_string(readBuffer.c_str());

		curl_easy_cleanup(curl);
		curl_slist_free_all(headerlist);

		if (http_code != 200) 
		{
			LOG(ERROR) << "<------------Lime Error------------";
			LOG(ERROR) << strTargetURL;
			LOG(ERROR) << strResourceJSON;
			LOG(ERROR) << readHeader;
			LOG(ERROR) << StringUtils::utf8_to_string(readBuffer.c_str());
			LOG(ERROR) << "------------Lime Error------------>";
			return false;
		}
		
		*result = json_loads(readBuffer.c_str(), 0, &error);
		return true;
	}
	return false;
}

#include <sstream>
#include <random>
#include <string>

unsigned int random_char()
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, 255);
	return dis(gen);
}

string generate_hex(const unsigned int len)
{
	std::stringstream ss;
	for (unsigned int i = 0; i < len; i++)
	{
		const auto rc = random_char();
		std::stringstream hexstream;
		hexstream << std::hex << rc;
		auto hex = hexstream.str();
		ss << (hex.length() < 2 ? '0' + hex : hex);
	}
	return ss.str();
}

string generate_lime_trace_id()
{
	return generate_hex(16);
}
