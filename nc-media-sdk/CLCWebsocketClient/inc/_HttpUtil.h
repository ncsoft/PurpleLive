#pragma once

#include "CLCWebsocketClient.h"
#include <functional>
#include <string>

using namespace std;

// 테스트를 위한 임시 함수
CLCWEBSOCKETCLIENT_API bool _HttpPost(const string& url, const wstring& uri, const string& info, std::function<bool(const string& msg)> handler);