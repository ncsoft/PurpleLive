#pragma once

#include "Singleton.h"

#if _DEBUG
	#define USE_STREAMING_STATS_INFO_LOG	0	// 송출 statistics 로그 활성화 여부
	#define USE_SOFTWARE_ENCODER_ONLY       0	// 소프트웨어 인코더 적용 여부
	#define USE_MAP_LOG_DEBUG				0	// MAP Log Debug모드에서 활성화 여부(RC망에서 지원함)
#else

#endif

struct GameCode
{
	static constexpr char* lms = "lms";
	static constexpr char* l2m = "l2m";
	static constexpr char* bns2 = "bns2";
	static constexpr char* tricksterm = "tricksterm";
	static constexpr char* h3 = "h3";
};

struct LanguageCode
{
	static constexpr char* ko_KR = "ko-KR";		//한국어
	static constexpr char* en_US = "en-US";		//영어
	static constexpr char* zh_TW = "zh-TW";		//중국어(번체)
	static constexpr char* zh_CN = "zh-CN";		//중국어(번체)
	static constexpr char* de_DE = "de-DE";		//독일어
	static constexpr char* es_ES = "es-ES";		//스페인어
	static constexpr char* fr_FR = "fr-FR";		//프랑스어
	static constexpr char* id_ID = "id-ID";		//인도네시아어
	static constexpr char* ja_JP = "ja-JP";		//일본어
	static constexpr char* pl_PL = "pl-PL";		//폴란드어
	static constexpr char* ru_RU = "ru-RU";		//러시아어
	static constexpr char* th_TH = "th-TH";		//태국어
	static constexpr char* ar_SA = "ar-SA";		//아랍어
	static constexpr char* vi_VN = "vi-VN";		// 베트남어
	static constexpr char* tr_TR = "tr-TR";		// 터키어
};

class AppConfig : public CommunityLiveCore::Singleton<AppConfig>
{
public:
	explicit AppConfig() {}
	virtual ~AppConfig() {}

	static constexpr char* APP_ID = "F22E67EA-1AEB-4066-9553-9DA70B473138";
	static constexpr char* APP_NAME = "PurpleLive";
	static constexpr char* APP_EXCUTE_EVENT_ID = "VideoChatApp_ExcuteEvent_CD4B45D5-A9E0-4674-9EC0-8A1BED0CB44C";
	static constexpr char* APP_DESTROY_EVENT_ID = "VideoChatApp_DestoryEvent_CD4B45D5-A9E0-4674-9EC0-8A1BED0CB44C";
	static constexpr char* APP_SHM_ID = "VideoChatSharedMemory_CD4B45D5-A9E0-4674-9EC0-8A1BED0CB44C";

	static constexpr int LIME_CLOSE_ROOM_TIMEOUT = 4000;
	static constexpr char* OPEN_SOURCE_POLICY_URL_RC = "https://***********";
	static constexpr char* OPEN_SOURCE_POLICY_URL_LIVE = "https://***********";

	static constexpr char* theme_light = "purplelive-light";
	static constexpr char* theme_purple = "purplelive-purple";
	static constexpr char* theme_dark = "purplelive-dark";
	static constexpr char* default_theme = theme_dark;

	static constexpr char* default_language = LanguageCode::en_US;
	static constexpr char* local_settings_path = "Purple/PurpleLive";
	static constexpr char* local_settings_file_name = "PurpleLive.ini";
	static constexpr char* purplelive_icon_path = ":/StartStreamDlg/purple.ico";
	static constexpr char* not_use_mic_device = "notUseMicDevice";

	static constexpr char* log_save_path = "Purple\\PurpleLive\\logs";
	static constexpr char* webview_cache_path = "Purple\\PurpleLive\\webview\\cache";
	static constexpr char* webview_persist_path = "Purple\\PurpleLive\\webview\\persist";
	static constexpr bool enable_log_error = true;
	static constexpr bool enable_log_info = true;
#ifdef _DEBUG
	static constexpr bool enable_log_debug = true;
#else
	static constexpr bool enable_log_debug = false;
#endif

	static constexpr float default_volume = 100.0f;
	static constexpr bool default_mute = false;

	int streaming_width				= 1280;
	int streaming_height			= 720;
	int streaming_bitrate			= 4 * 1000 * 1000;		// 4 Mbps
	int streaming_framerate			= 30;					// 30 frame
	bool is_rejoin_mediaroom		= true;					// MediaRoom이 남아있는 경우 rejoin 여부 (rejoin or close&create)
};