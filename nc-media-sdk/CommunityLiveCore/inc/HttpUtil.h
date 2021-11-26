#pragma once

#include "common.h"
#include <string>
#include <functional>

using namespace std;

// 테스트를 위한 임시 함수
COMMUNITY_LIVE_CORE_API bool HttpPost(const string& url, const wstring& uri, const string& info, std::function<bool(const string& msg)> handler);