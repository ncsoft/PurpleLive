
#include "HttpUtil.h"
#include "../../CLCWebsocketClient/inc/_HttpUtil.h"

#include <string>
using namespace std;

bool HttpPost(const string& url, const wstring& uri, const string& info, std::function<bool(const string& msg)> handler)
{
	return _HttpPost(url, uri, info, handler);
}
