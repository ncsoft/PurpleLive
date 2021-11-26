#pragma once

#include "CommunityLiveCoreConfig.h"
#include <string>
#include <limits>
#include <chrono>
#include <iostream>
#include <fstream>
#include "Singleton.h"
#include "../src/util.h"
#include "easylogging++.h"
using namespace std::chrono;

#define g_es		(EncoderStatistics::Instance())

// singleton
class EncoderStatistics : public CommunityLiveCore::Singleton<EncoderStatistics>
{
public:
	EncoderStatistics() { 
		InitFile();
		InitValue();
	}

	~EncoderStatistics() {
		UninitFile();
		LOG(INFO) << "[EncoderStatistics::~EncoderStatistics] Singleton";
	}

	void InitFile() {
#if USE_ENCODER_PERFORMANCE_LOGFILE
		if(!os_.is_open())
			os_.open("_encoder.log", std::ios_base::out | std::ios_base::app);
#endif
	}

	void UninitFile() {
#if USE_ENCODER_PERFORMANCE_LOGFILE
		if(os_.is_open())
			os_.close();
#endif
	}

	void InitValue() {
		playtime_ms_ = 0;
		capture_process_time_ = 0;
		encode_process_time_ = 0;

		capture_count_ = 0;
		capture_count_last_ = 0;
		encode_count_ = 0;
		encode_count_last_ = 0;
		encode_time_max_ = 0;
		sent_bytes_start_ = 0;

		auto now = std::chrono::high_resolution_clock::now();
		init_encode_time_ = now;
		log_time_last_ = now;
		filelog_time_last_ = now;
		capture_start_time_ = now;
		encode_start_time_ = now;
	}
	void SetIntervalEncoderLogFile(int millyseconds) {
		filelog_time_gap_ = millyseconds;
	}
	void SetEnableEncoderLogFile(bool enable) {
		ELOGI << format_string("[EncoderStatistics::EnableEncoderLogFile] enable_log_file_(%d)", enable);
		if(enable) {
			InitValue();
			InitFile();
		}
		else {
			InitValue();
			UninitFile();
		}
		enable_log_file_ = enable;
	}
	void InitCapture() {
		init_capture_time_ = std::chrono::high_resolution_clock::now();
	}
	void InitEncode(std::string name="") {
		InitValue();
		if(!name.empty())
			encoder_name_ = name;
	}	
	void CaptureFrameStart() {
		CheckPrintLog();

		auto now = std::chrono::high_resolution_clock::now();
		if(enable_delay_log_)
		{
			// 비디오 프레임 입력 지연 로그 (50ms 이상 지연 시)
			auto gap = std::chrono::duration_cast<std::chrono::milliseconds>(now-capture_start_time_);
			if(gap.count() >= 50)
				ELOGI << format_string("[EncoderStatistics::CaptureFrameStart][%s] input gap time(%d)", encoder_name_.c_str(), gap.count());
		}

		capture_start_time_ = now;
	}
	void CaptureFrameEnd() {
		auto now = std::chrono::high_resolution_clock::now();
		auto gap = std::chrono::duration_cast<std::chrono::milliseconds>(now-capture_start_time_);
		capture_process_time_ += gap.count();
		capture_count_++;
	}
	void EncodeFrameStart() {
		CheckPrintLog();

		auto now = std::chrono::high_resolution_clock::now();
		if(enable_delay_log_)
		{
			// 인코딩 콜백 지연 요청 로그 (50ms 이상 지연 시)
			auto gap = std::chrono::duration_cast<std::chrono::milliseconds>(now-encode_start_time_);
			if(gap.count() >= 50)
				ELOGI << format_string("[EncoderStatistics::EncodeFrameStart][%s] encode gap time(%d)", encoder_name_.c_str(), gap.count());
		}

		encode_start_time_ = now;
	}
	void EncodeFrameEnd() {
		auto now = std::chrono::high_resolution_clock::now();
		auto gap = std::chrono::duration_cast<std::chrono::milliseconds>(now-encode_start_time_);
		encode_process_time_ += gap.count();
		encode_count_++;
		if(capture_count_<encode_count_)
			capture_count_=encode_count_;

		if(gap.count() > encode_time_max_)
			encode_time_max_ = (int)gap.count();

		if(enable_delay_log_)
		{
			// 인코딩 후처리 지연 로그 (50ms 이상 소요 시)
			auto now = std::chrono::high_resolution_clock::now();
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now-encode_start_time_);
			if(elapsed.count() >= 50)
				ELOGI << format_string("[EncoderStatistics::EncodeFrameEnd][%s] encode elapsed time(%d)", encoder_name_.c_str(), elapsed.count());
		}
	}
	void SetSentBytes(uint64_t bytes_sent) {
		auto now = std::chrono::high_resolution_clock::now();
		auto gap = std::chrono::duration_cast<std::chrono::milliseconds>(now-init_encode_time_);
		
		if(sent_bytes_start_==0)
			sent_bytes_start_ = sent_bytes_;

		sent_bytes_ = bytes_sent - sent_bytes_start_;
		sent_bytes_per_sec_ = sent_bytes_ * 1000 / gap.count();
	}
	void CheckPrintLog() {
		auto now = std::chrono::high_resolution_clock::now();
		auto gap = std::chrono::duration_cast<std::chrono::milliseconds>(now-log_time_last_);
		if(gap.count() >= log_time_gap_) {
			PrintLog();
			log_time_last_ = now;
		}
	}
	std::string GetEncoderName() {
		return encoder_name_;
	}
	uint64_t GetCaptureCount() {
		return capture_count_;
	}
	uint64_t GetPlayTime() {
		return (int)(playtime_ms_/1000);
	}
	uint64_t GetPlayTimeMillySeconds() {
		return playtime_ms_;
	}
	std::string getCurrentDateTime(bool useLocalTime) {
		time_t ttNow = time(0);
		tm tmNow;
		tm * ptmNow = &tmNow;

		if (useLocalTime)
			localtime_s(&tmNow, &ttNow);
		else
			gmtime_s(&tmNow, &ttNow);

		std::string date = format_string("%4d-%02d-%02d", 1900+ptmNow->tm_year, (1+ptmNow->tm_mon), ptmNow->tm_mday);
		std::string time = format_string("%02d:%02d:%02d", ptmNow->tm_hour, ptmNow->tm_min, ptmNow->tm_sec);
		return format_string("%s %s", date.c_str(), time.c_str());
	}
	void PrintLog() {
		auto now = std::chrono::high_resolution_clock::now();
		auto gap = std::chrono::duration_cast<std::chrono::milliseconds>(now-init_encode_time_);
		playtime_ms_ = gap.count();
		int playtime = (int)(playtime_ms_/1000);

		if(playtime==0) return;
		if(encode_count_==0) return;
		if(capture_count_==0) return;

		if(enable_log_)
		{
			ELOGI << format_string("------------------------------------------------------------------------------------");
			ELOGI << format_string("[EncoderStatistics][%s][AVG__] playtime(%d) capture(%.2lf) encode(%.2lf) diff(%.2lf) encode_time/s(%.2lf) encode_time/f(%.2lf) encode_time_max(%d)", encoder_name_.c_str(), playtime, (double)capture_count_/playtime, (double)encode_count_/playtime, (double)(capture_count_-encode_count_)/playtime, (double)encode_process_time_/playtime, (double)encode_process_time_/encode_count_, encode_time_max_);
			ELOGI << format_string("[EncoderStatistics][%s][CURR_] playtime(%d) capture(%lld) encode(%lld) diff(%lld)", encoder_name_.c_str(), playtime, (capture_count_-capture_count_last_)/playtime, (encode_count_-encode_count_last_)/playtime, (capture_count_-capture_count_last_)-(encode_count_-encode_count_last_)/playtime);
			ELOGI << format_string("[EncoderStatistics][%s][TOTAL] playtime(%d) capture(%lld) encode(%lld) diff(%lld) encode_time(%lld)", encoder_name_.c_str(), playtime, capture_count_, encode_count_, capture_count_-encode_count_, encode_process_time_);
			ELOGI << format_string("[EncoderStatistics][%s][STAT_] playtime(%d) sent_bytes(%lld) sent_bytes/s(%lldKbps)", encoder_name_.c_str(), playtime, sent_bytes_*8/1000, sent_bytes_per_sec_*8/1000);
			ELOGI << format_string("[EncoderStatistics][%s][RATE_] playtime(%d) droprate(%03.2lf%%) dropcount/s(%.2lf) encodetimerate(%03.2lf%%)", encoder_name_.c_str(), playtime, (double)(capture_count_-encode_count_)/capture_count_, (double)(capture_count_-encode_count_)/playtime, (double)encode_process_time_/(playtime*10));
			ELOGI << format_string("------------------------------------------------------------------------------------");
		}

#if USE_ENCODER_PERFORMANCE_LOGFILE
		if(enable_log_file_ && os_.is_open())
		{
			auto gap = std::chrono::duration_cast<std::chrono::milliseconds>(now-filelog_time_last_);
			bool save_log = filelog_time_gap_ <= gap.count();
			if(save_log)
			{
				std::string datetime{getCurrentDateTime(true)};
				os_ << format_string("------------------------------------------------------------------------------------\n");
				os_ << format_string("%s [EncoderStatistics][%s][AVG__] playtime(%d) capture(%.2lf) encode(%.2lf) diff(%.2lf) encode_time/s(%.2lf) encode_time/f(%.2lf) encode_time_max(%d)\n", datetime.c_str(), encoder_name_.c_str(), playtime, (double)capture_count_/playtime, (double)encode_count_/playtime, (double)(capture_count_-encode_count_)/playtime, (double)encode_process_time_/playtime, (double)encode_process_time_/encode_count_, encode_time_max_);
				os_ << format_string("%s [EncoderStatistics][%s][CURR_] playtime(%d) capture(%lld) encode(%lld) diff(%lld)\n", datetime.c_str(), encoder_name_.c_str(), playtime, capture_count_last_/playtime, encode_count_last_/playtime, (capture_count_last_-encode_count_last_)/playtime);
				os_ << format_string("%s [EncoderStatistics][%s][TOTAL] playtime(%d) capture(%lld) encode(%lld) diff(%lld) encode_time(%lld)\n", datetime.c_str(), encoder_name_.c_str(), playtime, capture_count_, encode_count_, capture_count_-encode_count_, encode_process_time_);
				os_ << format_string("%s [EncoderStatistics][%s][STAT_] playtime(%d) sent_bytes(%lld) sent_bytes/s(%lldKbps)\n", datetime.c_str(), encoder_name_.c_str(), playtime, sent_bytes_*8/1000, sent_bytes_per_sec_*8/1000);
				os_ << format_string("%s [EncoderStatistics][%s][RATE_] playtime(%d) droprate(%03.2lf%%) dropcount/s(%.2lf) encodetimerate(%03.2lf%%)\n\n\n", datetime.c_str(), encoder_name_.c_str(), playtime, (double)(capture_count_-encode_count_)/capture_count_, (double)(capture_count_-encode_count_)/playtime, (double)encode_process_time_/(playtime*10));
				filelog_time_last_ = now;
				SetEnableEncoderLogFile(false);
			}
		}
#endif

		capture_count_last_ = capture_count_;
		encode_count_last_ = encode_count_;
	}

private:
	
	bool enable_log_ = true;
	bool enable_log_file_ = false;
	bool enable_delay_log_ = false;
	int32_t log_time_gap_ = 2000;
	int32_t filelog_time_gap_ = 5 * 1000;

	std::ofstream os_;
	std::string encoder_name_ = "None";

	high_resolution_clock::time_point init_capture_time_;
	high_resolution_clock::time_point init_encode_time_;

	high_resolution_clock::time_point log_time_last_;
	high_resolution_clock::time_point filelog_time_last_;
	high_resolution_clock::time_point capture_start_time_;
	high_resolution_clock::time_point encode_start_time_;

	uint64_t playtime_ms_ = 0;
	uint64_t capture_process_time_ = 0;
	uint64_t encode_process_time_ = 0;

	uint64_t sent_bytes_ = 0;
	uint64_t sent_bytes_start_ = 0;
	uint64_t sent_bytes_per_sec_ = 0;

	uint64_t capture_count_ = 0;
	uint64_t capture_count_last_ = 0;
	uint64_t encode_count_ = 0;
	uint64_t encode_count_last_ = 0;

	uint32_t encode_time_max_ = 0;
};
