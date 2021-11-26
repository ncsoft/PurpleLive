#pragma once
#include <NCCaptureSDK.h>
#include <string>
#include <map>

// openssl
#include <bio.h>
#include <asn1.h>
#include <rsa.h>
#include <err.h>
#include <ssl.h>
#include <pem.h>
#include <sha.h>
#include <hmac.h>
#include <evp.h>
#include <buffer.h>
#include <aes.h>
#include <des.h>
#include <evp.h>

#include "lime-defines.h"

using namespace std;

const string si_rsa_private_key = R"(-----BEGIN PRIVATE KEY-----
***********
-----END PRIVATE KEY-----)";

const string service_api_domain_rc = "https://***********";
const string service_api_domain_op = "https://***********";
const string service_api_domain_sb = "https://***********";
const string service_api_domain_live = "https://***********";

#define SIM_RESPONSE_BUFFER_SIZE	4096

// 서비스 점검중인지 체크
class ServiceInspectionManager : public NCCaptureSDK::Singleton<ServiceInspectionManager>
{
public:
	ServiceInspectionManager();
	~ServiceInspectionManager();

	bool	IsInspection(eServiceNetwork sn, string& msg);

private:
	const string	get_language_code();
	const string	get_service_api_domain(eServiceNetwork sn);
	bool			get_key(eServiceNetwork sn, string& response);
	bool			post_maintenance(eServiceNetwork sn, string& response);
	
	bool			encode_base64(const unsigned char* input, int length, string& out);
	int				decode_base64(const unsigned char* input, int length, char** out);

	RSA*			createRSA(unsigned char* key, bool bPublic);
	int				private_decrypt(unsigned char* enc_data, int data_len, unsigned char* key, unsigned char* decrypted);
	int				public_decrypt(unsigned char* enc_data, int data_len, unsigned char* key, unsigned char* decrypted);
	int				aes128_decrypt(unsigned char* input, unsigned char* output, int size);

	string			m_aes_key;
};

