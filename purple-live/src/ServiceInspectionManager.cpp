#include "ServiceInspectionManager.h"
#include <curl.h>
#include <json.h>
#include <iostream>
#include <fstream>
#include "VideoChatApp.h"
#include "AppUtils.h"
#include "AppString.h"
#include <atlconv.h>
#include "easylogging++.h"
#include "MAPManager.h"

#include <stdlib.h>
void printLastError(char *msg)
{
	char err[130];
	ERR_load_crypto_strings();
	ERR_error_string(ERR_get_error(), err);
	printf("%s ERROR: %s\n", msg, err);
//	free(err);
}

ServiceInspectionManager::ServiceInspectionManager()
{
}

ServiceInspectionManager::~ServiceInspectionManager()
{
	elogd("[ServiceInspectionManager::~ServiceInspectionManager] Singleton");
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

static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
	void *userp)
{
	((std::string *)userp)->append((char *)contents, size * nmemb);
	return size * nmemb;
}

bool ServiceInspectionManager::IsInspection(eServiceNetwork sn, string& msg)
{
	string response;
	if (false == get_key(sn, response))
	{
		return false;
	}
	m_aes_key = response;

	LOG(DEBUG) << m_aes_key;
	response = "";
	if (false == post_maintenance(sn, response))
	{
		return false;
	}
	msg = response;

	return true;
}

const string ServiceInspectionManager::get_language_code()
{
	//en_US, ko_KR와 같은 형태, 언어(ISO 639 - 1 alpha - 2 codes) + "_" + 국가
	std::string code = App()->GetLocalLanguage().toStdString();
	std::replace(code.begin(), code.end(), '-', '_');
	return code;
}

const string ServiceInspectionManager::get_service_api_domain(eServiceNetwork sn)
{
	switch (sn) 
	{
	case eSN_RC:
		return service_api_domain_rc;

	case eSN_OP:
		assert(0);
		return service_api_domain_rc;

	case eSN_SB:
		return service_api_domain_sb;

	default:
	case eSN_LIVE:
		return service_api_domain_live;
	}
}

bool ServiceInspectionManager::get_key(eServiceNetwork sn, string& response)
{
	CURL *curl;
	CURLcode res;

	std::string strResourceJSON;
	std::string readBuffer;
	std::string readHeader;

	string strTargetURL = get_service_api_domain(sn) + "/app/key";
	string url = string_format("%s?app_id=%s&app_config_type=%d", strTargetURL.c_str(), AppConfig::APP_ID, 6);
	LOG(DEBUG) << "------------Send------------";
	LOG(DEBUG) << url;

	struct curl_slist *headerlist = nullptr;
	headerlist = curl_slist_append(headerlist, "Content-Type: application/json;charset=UTF-8");

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);

		// 결과 기록
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&readBuffer);
		curl_easy_setopt(curl, CURLOPT_WRITEHEADER, (void *)&readHeader);

		res = curl_easy_perform(curl);

		if (res != CURLE_OK)
		{
			response = AppUtils::format("[ServiceInspaectionManager] curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			eloge(response.c_str());
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
			LOG(ERROR) << "<------------Service API Server Error------------";
			LOG(ERROR) << strTargetURL;
			LOG(ERROR) << strResourceJSON;
			LOG(ERROR) << readHeader;
			LOG(ERROR) << StringUtils::utf8_to_string(readBuffer.c_str());
			LOG(ERROR) << "------------Service API Server Error------------>";
			response = AppUtils::format("[ServiceInspaectionManager] http error: %d\n", http_code);
			return false;
		}

		json_error_t error;
		json_t *root = json_loads(readBuffer.c_str(), 0, &error);

		if (!root)
		{
			response = AppUtils::format("[ServiceInspaectionManager] error: on line %d: %s\n", error.line, error.text);
			eloge(response.c_str());
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
		// value
		json_t* jsonValue = json_object_get(root, "value");
		if (!json_is_string(jsonValue))
		{
			json_decref(root);
			response = AppUtils::format("[ServiceInspaectionManager] json parsing error\n");
			eloge(response.c_str());
			return false;
		}

		string encValue = json_string_value(jsonValue);

		// decode base64
		char* pDecValue = nullptr;
		int decLen = decode_base64((const unsigned char*)encValue.c_str(), encValue.length(), &pDecValue);
		
		// decrypt value
		unsigned char decrypted[SIM_RESPONSE_BUFFER_SIZE] = {};

		int decrypted_len = private_decrypt((unsigned char*)pDecValue, decLen, (unsigned char*)si_rsa_private_key.c_str(), decrypted);
		free(pDecValue);
		if (-1 == decrypted_len)
		{
			response = AppUtils::format("[ServiceInspaectionManager] decrypt error\n");
			eloge(response.c_str());
			return false;
		}
		else
		{
			response = (char*)decrypted;
			return true;
		}
		// parse --------------------------------------------------
	}

	response = AppUtils::format("[ServiceInspaectionManager] unknown error\n");
	eloge(response.c_str());

	return false;
}

bool ServiceInspectionManager::post_maintenance(eServiceNetwork sn, string& response)
{
	CURL *curl;
	CURLcode res;

	std::string strResourceJSON;
	std::string readBuffer;
	std::string readHeader;

	string strTargetURL = get_service_api_domain(sn) + "/app/policy/maintenance";
	LOG(DEBUG) << "------------Send------------";
	LOG(DEBUG) << strTargetURL;

	struct curl_slist *headerlist = nullptr;
	headerlist = curl_slist_append(headerlist, "Content-Type: application/json;charset=UTF-8");

	json_t *root = json_object();
	json_object_set_new(root, "app_id", json_string(AppConfig::APP_ID));
	json_object_set_new(root, "app_config_type", json_integer(6));	// Windows
	json_object_set_new(root, "platform_type", json_integer(1));		// NC Mobile SDK
	json_object_set_new(root, "app_version", json_string("MASTER"));
	json_object_set_new(root, "language_code", json_string(get_language_code().c_str()));
	string adid = MAPManager::Instance()->GetAdid() + "|";
	json_object_set_new(root, "check_value", json_string(adid.c_str()));
	json_object_set_new(root, "is_maintenance_time_required", json_integer(1));	//시간정보 포함
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
			response = AppUtils::format("[ServiceInspaectionManager] curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			eloge(response.c_str());
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
			LOG(ERROR) << "<------------Service API Server Error------------";
			LOG(ERROR) << strTargetURL;
			LOG(ERROR) << strResourceJSON;
			LOG(ERROR) << readHeader;
			LOG(ERROR) << StringUtils::utf8_to_string(readBuffer.c_str());
			LOG(ERROR) << "------------Service API Server Error------------>";
			response = AppUtils::format("[ServiceInspaectionManager] http error: %d\n", http_code);
			return false;
		}

		json_error_t error;
		json_t *root = json_loads(readBuffer.c_str(), 0, &error);

		if (!root)
		{
			response = AppUtils::format("[ServiceInspaectionManager] error: on line %d: %s\n", error.line, error.text);
			eloge(response.c_str());
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
		// value
		json_t* jsonValue = json_object_get(root, "value");
		if (!json_is_string(jsonValue))
		{
			json_decref(root);
			response = AppUtils::format("[ServiceInspaectionManager] json parsing error\n");
			eloge(response.c_str());
			return false;
		}

		string encValue = json_string_value(jsonValue);

		// decode base64
		char* pDecValue = nullptr;
		int decLen = decode_base64((const unsigned char*)encValue.c_str(), encValue.length(), &pDecValue);
		LOG(DEBUG) << pDecValue;

		// decrypt value
		char result[SIM_RESPONSE_BUFFER_SIZE] = {};
		aes128_decrypt((unsigned char*)pDecValue, (unsigned char*)result, decLen);
		free(pDecValue);

		char* end = strrchr((char*)result, '}');
		if (nullptr != end)
		{
			if (end != &result[SIM_RESPONSE_BUFFER_SIZE-1])
			{
				*(end + 1) = '\0';
			}
		}

 		response = result;
		return true;
		// parse --------------------------------------------------
	}
	return false;
}

bool ServiceInspectionManager::encode_base64(const unsigned char* input, int input_length, string& out)
{
	BUF_MEM *bptr;

	BIO* b64 = BIO_new(BIO_f_base64());
	BIO* bmem = BIO_new(BIO_s_mem());
	b64 = BIO_push(b64, bmem);
	BIO_write(b64, input, input_length);
	BIO_flush(b64);
	BIO_get_mem_ptr(b64, &bptr);

	char *buff = (char *)malloc(bptr->length);
	memcpy(buff, bptr->data, bptr->length - 1);
	buff[bptr->length - 1] = 0;

	BIO_free_all(b64);

	out = buff;
	free(buff);

	return true;
}

int ServiceInspectionManager::decode_base64(const unsigned char* input, int length, char** out)
{
	*out = (char *)malloc(length);
	memset(*out, 0, length);

	BIO* b64 = BIO_new(BIO_f_base64());
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	BIO* bmem = BIO_new_mem_buf(input, length);
	bmem = BIO_push(b64, bmem);

	int decodedLen = BIO_read(bmem, *out, length);

	BIO_free_all(bmem);
	
	return decodedLen;
}

RSA * ServiceInspectionManager::createRSA(unsigned char * key, bool bPublic)
{
	RSA* rsa = NULL;
	BIO* bio = BIO_new_mem_buf(key, -1);
	if (nullptr == bio)
	{
		printf("Failed to create key BIO");
		return 0;
	}
	if (bPublic)
	{
		rsa = PEM_read_bio_RSA_PUBKEY(bio, &rsa, NULL, NULL);
	}
	else
	{
		rsa = PEM_read_bio_RSAPrivateKey(bio, &rsa, NULL, NULL);
	}
	if (rsa == NULL)
	{
		printf("Failed to create RSA");
	}

	return rsa;
}

int ServiceInspectionManager::private_decrypt(unsigned char* enc_data, int data_len, unsigned char* key, unsigned char* decrypted)
{
	RSA* rsa = createRSA(key, 0);
	int  result = RSA_private_decrypt(data_len, enc_data, decrypted, rsa, RSA_PKCS1_PADDING);
	if (result == -1)
	{
		printLastError("rsa private decrypt error");
	}
	RSA_free(rsa);
	return result;
}

int ServiceInspectionManager::public_decrypt(unsigned char * enc_data, int data_len, unsigned char * key, unsigned char * decrypted)
{
	RSA* rsa = createRSA(key, 1);
	int  result = RSA_public_decrypt(data_len, enc_data, decrypted, rsa, RSA_PKCS1_PADDING);

	RSA_free(rsa);
	return result;
}

int ServiceInspectionManager::aes128_decrypt(unsigned char* input, unsigned char* output, int size)
{
	AES_KEY aes_key;
	unsigned char iv_aes[AES_BLOCK_SIZE];
	memcpy(iv_aes, m_aes_key.c_str(), AES_BLOCK_SIZE);

	AES_set_decrypt_key((const unsigned char*)m_aes_key.c_str(), m_aes_key.length()*8, &aes_key);
	AES_cbc_encrypt(input, output, size, &aes_key, iv_aes, AES_DECRYPT);
	return 0;
}
