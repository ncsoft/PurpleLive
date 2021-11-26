#pragma once

#include <string>
#include <vector>
#include <jansson.h>
#include "lime-defines.h"

using namespace std;


bool LoginWithJWToken(const char* gameUserID, const char* token, const char* deviceId, StompInfo& info, eServiceNetwork sn, const char* appVersion, const char* languageCode);
bool GetSubscriptionInfo(const char* token, StompInfo& info, eServiceNetwork sn, const char* appVersion, const char* languageCode);
eGetCharacterErrorType GetCharacterList(const char* gameCode, const char* token, vector<GameUserInfo> &list, eServiceNetwork sn, const char* appVersion, const char* languageCode);
bool CreateChatting(const char* token, vector<GameUserInfo> &list, eServiceNetwork sn, const char* appVersion, json_t** result, json_error_t& error, const char* params);
bool GetInvitationUserList(const char* token, vector<GameUserInfo> &list, eServiceNetwork sn, const char* appVersion, json_t** result, json_error_t& error, const char* gameUserId);
bool GetGuildUserList(const char* token, vector<GameUserInfo> &list, eServiceNetwork sn, const char* appVersion, json_t** result, json_error_t& error);
bool GetShareTargetList(const char* token, vector<GameUserInfo> &list, eServiceNetwork sn, const char* appVersion, json_t** result, json_error_t& error);

// uuid generate
unsigned int random_char();
string generate_hex(const unsigned int len);
string generate_lime_trace_id();

//
string json_get_string_value(json_t* obj, string key);
int json_get_integer_value(json_t* obj, string key);
bool json_get_bool_value(json_t* obj, string key);

//
template<typename... Args>
std::string string_format(const std::string &format, Args... args);
