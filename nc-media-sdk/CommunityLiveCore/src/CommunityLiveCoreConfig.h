#pragma once

#define USE_ENCODER_PERFORMANCE_LOGFILE		0
#define USE_ENCODER_PERFORMANCE_TEST		0

class CommunityLiveCoreConfig {
public:	
	static constexpr bool USE_STAT_INFO = true;
	static constexpr bool USE_STAT_INFO_DETAIL = true;

	static constexpr bool USE_STAT_THREAD = false;
	static constexpr int TIMER_GET_STAT_INTERVAL = 30;
	static constexpr int GET_STAT_DETAIL_LOG_INTERVAL = 2000;	// 2 seconds
};
