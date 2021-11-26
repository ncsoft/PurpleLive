
#include "../inc/_HttpUtil.h"

#include <json/json.h>

//#define _NO_ASYNCRTIMP
#include "cpprest/http_client.h"

using namespace web::http;
using namespace web::http::client;
using namespace utility;

using namespace std;

bool _HttpPost(const string& url, const wstring& uri, const string& info, std::function<bool(const string& msg)> handler)
{
	cout << info << endl;

	http_client_config cfg;
	cfg.set_validate_certificates(false);
	http_client hcli(conversions::to_utf16string(url), cfg);
	http_request hreq(methods::POST);
	hreq.set_request_uri(uri);
	hreq.set_body(info, "application/json");

	bool result = true;

	try
	{
		hcli.request(hreq).then([&result, handler](http_response rsp)
		{
			auto bodystream = rsp.extract_string();
			string body = conversions::utf16_to_utf8(bodystream.get().c_str());
			cout << body << endl;

			if (rsp.status_code() != status_codes::OK)
			{
				result = false;
				return;
			}

			result = handler(body);
			return;
		}).wait();
	}
	catch (const std::exception &e)
	{
		string s = e.what();
		cout << s << endl;
		return false;
	}

	return result;

//	return true;
}

