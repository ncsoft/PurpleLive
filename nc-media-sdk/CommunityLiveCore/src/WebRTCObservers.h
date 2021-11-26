#pragma once

#include "CommunityLiveCoreConfig.h"
#include "EncoderStatistics.h"
#include "api/peer_connection_interface.h"
#include "api/stats/rtcstats_objects.h"
#include "easylogging++.h"
#include "MediaRoomBase.h"
#include <string>
#include <iostream>
#include "util.h"

using namespace std;

/*
 * WebRTC 옵저버 베이스
 */
class WebRTC_ObserverBase 
{
public:
	WebRTC_ObserverBase()
	{
		m_peer = nullptr;
		m_peerID = "";
		m_pNotifier = nullptr;
	}

	void Setup(webrtc::PeerConnectionInterface& peer, WebRTCPeerNotifier* pNotifier, const string& peerID)
	{
		m_peer = &peer;
		m_peerID = peerID;
		m_pNotifier = pNotifier;
	}
protected:
	rtc::scoped_refptr<webrtc::PeerConnectionInterface> m_peer;	// owner peer
	string				m_peerID;											// owner peer ID
	WebRTCPeerNotifier* m_pNotifier;
};

/*
 * SDP 설정에 대한 옵저버
 */
class WebRTC_SetSDP_Observer : public WebRTC_ObserverBase, public webrtc::SetSessionDescriptionObserver
{
public:

	virtual void OnSuccess() override
	{
		RTC_LOG(INFO) << __FUNCTION__;
		ELOGD << "local/remote media success";
	}
	
	virtual void OnFailure(webrtc::RTCError error) override
	{
		RTC_LOG(INFO) << __FUNCTION__ << " " << ToString(error.type()) << ": " << error.message();
		ELOGE << "local/remote media fail";
	}
};

/*
 * SDP 생성에 대한 옵저버
 *
 * 내부적으로 SDP 설정 옵저버 포함
 */
class WebRTC_SDP_Observer : public WebRTC_ObserverBase, public webrtc::CreateSessionDescriptionObserver
{

	// CreateSessionDescriptionObserver =====================================================
	virtual void OnSuccess(webrtc::SessionDescriptionInterface* desc) override 
	{
		RTC_LOG(INFO) << __FUNCTION__;
		if (!m_setSDPObserver)
		{
			m_setSDPObserver = new rtc::RefCountedObject<WebRTC_SetSDP_Observer>();
			if (!m_setSDPObserver)
			{
				ELOGE << "m_SetSdpObsvr alloc fail";
				assert(m_setSDPObserver.get());
				return;
			}
			m_setSDPObserver->Setup(*m_peer, m_pNotifier, m_peerID);
		}

		m_peer->SetLocalDescription(m_setSDPObserver, desc);

		if (!m_pNotifier)
			return;

		string msg;
		desc->ToString(&msg);
//		IF_LOG(plog::debug)
		{
			ELOGD << "====================================================";
			ELOGD << "Offering session description: ";
			ELOGD << msg;
			ELOGD << "====================================================";
		}
		m_pNotifier->OnSdp(m_peerID, SdpTypeToString(desc->GetType()), msg);
	}

	virtual void OnFailure(webrtc::RTCError error) override 
	{
		RTC_LOG(INFO) << __FUNCTION__ << " " << ToString(error.type()) << ": " << error.message();
		m_pNotifier->OnSdpFailed(m_peerID);
	}
//	virtual void OnFailure(const std::string& error) override {}
	//=======================================================================================

protected:
	rtc::scoped_refptr<WebRTC_SetSDP_Observer> m_setSDPObserver;
};

/*
 * Stat 정보에 대한 옵저버
 */
class WebRTC_Stat_Observer : public WebRTC_ObserverBase, public webrtc::RTCStatsCollectorCallback
{
	high_resolution_clock::time_point m_start_time;
	high_resolution_clock::time_point m_last_log_time;

	bool m_enable_log = false;

public:
	void SetEnableLog(bool enable) { m_enable_log = enable; }

	void OnStatsDelivered(const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) override 
	{
		if (!m_pNotifier) return;
		
		bool isEnableLog = false;
		auto now = std::chrono::high_resolution_clock::now();
		auto gap = std::chrono::duration_cast<std::chrono::milliseconds>(now-m_last_log_time);
		if (gap.count() >= CommunityLiveCoreConfig::GET_STAT_DETAIL_LOG_INTERVAL)
		{
			isEnableLog = m_enable_log;
			m_last_log_time = now;
			if (m_start_time == high_resolution_clock::time_point(0ns))
			{
				m_start_time = now;
			}
		}

		MediaStatInfo msi;
		if(CommunityLiveCoreConfig::USE_STAT_INFO) 
		{
			for (auto& mediaStats : report->GetStatsOfType<webrtc::RTCMediaStreamTrackStats>()) 
			{
				bool isAudio = mediaStats->audio_level.is_defined();
				bool isVideo = mediaStats->frame_width.is_defined();
				if(isAudio==false && isVideo==false) continue;

				for (auto& streamStats : report->GetStatsOfType<webrtc::RTCOutboundRTPStreamStats>()) 
				{
					if(!streamStats->bytes_sent.is_defined()) continue;

					msi.packets_sent = (!streamStats->packets_sent.is_defined()) ? 0 : *(streamStats->packets_sent.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
					msi.target_bitrate = (!streamStats->target_bitrate.is_defined()) ? 0 : *(streamStats->target_bitrate.cast_to<const webrtc::RTCStatsMember<double>>());
					msi.frames_encoded = (!streamStats->frames_encoded.is_defined()) ? 0 : *(streamStats->frames_encoded.cast_to<const webrtc::RTCStatsMember<uint32_t>>());

					if(isAudio)
					{	
						msi.audio_bytes_sent = (!streamStats->bytes_sent.is_defined()) ? 0 : *(streamStats->bytes_sent.cast_to<const webrtc::RTCStatsMember<uint64_t>>());
						//ELOGD << format_string("[A] bytes_sent(%10ld)", msi.bytes_sent);
						//ELOGD << format_string("[A] bytes_sent(%10ld) packets_sent(%10d) target_bitrate(%f) frames_encoded(%d)", msi.bytes_sent, msi.packets_sent, msi.target_bitrate, msi.frames_encoded);
					}
					else if(isVideo)
					{
						msi.video_bytes_sent = (!streamStats->bytes_sent.is_defined()) ? 0 : *(streamStats->bytes_sent.cast_to<const webrtc::RTCStatsMember<uint64_t>>());
						msi.frames_sent = (!mediaStats->frames_sent.is_defined()) ? 0 : *(mediaStats->frames_sent.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
						msi.frames_dropped = (!mediaStats->frames_dropped.is_defined()) ? 0 : *(mediaStats->frames_sent.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
						//ELOGD << format_string("[V] bytes_sent(%10ld) frames_dropped(%d/%d)", msi.bytes_sent, msi.frames_dropped, msi.frames_sent);
						//ELOGD << format_string("[V] bytes_sent(%10ld) packets_sent(%10d) target_bitrate(%f) frames_encoded(%d) frames_dropped(%d/%d)", msi.bytes_sent, msi.packets_sent, msi.target_bitrate, msi.frames_encoded, msi.frames_dropped, msi.frames_sent);
					}
				}	
			}
		}

		MediaStatDetailInfo msdi;
		if (CommunityLiveCoreConfig::USE_STAT_INFO_DETAIL)
		{
			if(isEnableLog)
				ELOGD << format_string("=============================================================================================================");

			for (auto& iceCandidatePareStats : report->GetStatsOfType<webrtc::RTCIceCandidatePairStats>()) {
				msdi.timestamp_us = iceCandidatePareStats->timestamp_us();
				msdi.bytes_sent = (!iceCandidatePareStats->bytes_sent.is_defined()) ? 0 : *(iceCandidatePareStats->bytes_sent.cast_to<const webrtc::RTCStatsMember<uint64_t>>());
				msdi.bytes_sent = (!iceCandidatePareStats->bytes_sent.is_defined()) ? 0 : *(iceCandidatePareStats->bytes_sent.cast_to<const webrtc::RTCStatsMember<uint64_t>>());
				msdi.bytes_received = (!iceCandidatePareStats->bytes_received.is_defined()) ? 0 : *(iceCandidatePareStats->bytes_received.cast_to<const webrtc::RTCStatsMember<uint64_t>>());
				msdi.total_round_trip_time = (!iceCandidatePareStats->total_round_trip_time.is_defined()) ? 0 : *(iceCandidatePareStats->total_round_trip_time.cast_to<const webrtc::RTCStatsMember<double>>());
				msdi.current_round_trip_time = (!iceCandidatePareStats->current_round_trip_time.is_defined()) ? 0 : *(iceCandidatePareStats->current_round_trip_time.cast_to<const webrtc::RTCStatsMember<double>>());
				msdi.available_outgoing_bitrate = (!iceCandidatePareStats->available_outgoing_bitrate.is_defined()) ? 0 : *(iceCandidatePareStats->available_outgoing_bitrate.cast_to<const webrtc::RTCStatsMember<double>>());
				msdi.available_incoming_bitrate = (!iceCandidatePareStats->available_incoming_bitrate.is_defined()) ? 0 : *(iceCandidatePareStats->available_incoming_bitrate.cast_to<const webrtc::RTCStatsMember<double>>());
				msdi.requests_received = (!iceCandidatePareStats->requests_received.is_defined()) ? 0 : *(iceCandidatePareStats->requests_received.cast_to<const webrtc::RTCStatsMember<uint64_t>>());
				msdi.requests_sent = (!iceCandidatePareStats->requests_sent.is_defined()) ? 0 : *(iceCandidatePareStats->requests_sent.cast_to<const webrtc::RTCStatsMember<uint64_t>>());
				msdi.responses_received = (!iceCandidatePareStats->responses_received.is_defined()) ? 0 : *(iceCandidatePareStats->responses_received.cast_to<const webrtc::RTCStatsMember<uint64_t>>());
				msdi.responses_sent = (!iceCandidatePareStats->responses_sent.is_defined()) ? 0 : *(iceCandidatePareStats->responses_sent.cast_to<const webrtc::RTCStatsMember<uint64_t>>());
				msdi.retransmissions_received = (!iceCandidatePareStats->retransmissions_received.is_defined()) ? 0 : *(iceCandidatePareStats->retransmissions_received.cast_to<const webrtc::RTCStatsMember<uint64_t>>());
				msdi.retransmissions_sent = (!iceCandidatePareStats->retransmissions_sent.is_defined()) ? 0 : *(iceCandidatePareStats->retransmissions_sent.cast_to<const webrtc::RTCStatsMember<uint64_t>>());

				if(isEnableLog)
				{
					//ELOGD << format_string("[RTCIceCandidatePairStats][I] instance(0x%p) bytes_sent(%lld) available_outgoing_bitrate(%f)", &iceCandidatePareStats, msdi.bytes_sent, msdi.available_outgoing_bitrate);
					ELOGD << format_string("[RTCIceCandidatePairStats][I] instance(0x%p) bytes_sent(%lld) bytes_received(%lld)", &iceCandidatePareStats, msdi.bytes_sent, msdi.bytes_received);
					ELOGD << format_string("[RTCIceCandidatePairStats][I] instance(0x%p) total_round_trip_time(%f) current_round_trip_time(%f)", &iceCandidatePareStats, msdi.total_round_trip_time, msdi.current_round_trip_time);
					ELOGD << format_string("[RTCIceCandidatePairStats][I] instance(0x%p) available_outgoing_bitrate(%f) available_incoming_bitrate(%f)", &iceCandidatePareStats, msdi.available_outgoing_bitrate, msdi.available_incoming_bitrate);
					ELOGD << format_string("[RTCIceCandidatePairStats][I] instance(0x%p) requests_received(%lld) requests_sent(%lld) responses_received(%lld) responses_sent(%lld) retransmissions_received(%lld) retransmissions_sent(%lld) ", 
						&iceCandidatePareStats, msdi.requests_received, msdi.requests_sent, msdi.responses_received, msdi.responses_sent, msdi.retransmissions_received, msdi.retransmissions_sent);
				}
			}

			for (auto& outboundStreamStats : report->GetStatsOfType<webrtc::RTCOutboundRTPStreamStats>()) 
			{
				std::string media_type = (!outboundStreamStats->media_type.is_defined()) ? "" : *(outboundStreamStats->media_type.cast_to<const webrtc::RTCStatsMember<std::string>>());
				msdi.mosi.media_source_id = (!outboundStreamStats->media_source_id.is_defined()) ? "" : *(outboundStreamStats->media_source_id.cast_to<const webrtc::RTCStatsMember<std::string>>());
				msdi.mosi.remote_id = (!outboundStreamStats->remote_id.is_defined()) ? "" : *(outboundStreamStats->remote_id.cast_to<const webrtc::RTCStatsMember<std::string>>());
				msdi.mosi.rid = (!outboundStreamStats->rid.is_defined()) ? "" : *(outboundStreamStats->rid.cast_to<const webrtc::RTCStatsMember<std::string>>());

				msdi.mosi.packets_sent = (!outboundStreamStats->packets_sent.is_defined()) ? 0 : *(outboundStreamStats->packets_sent.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
				msdi.mosi.retransmitted_packets_sent = (!outboundStreamStats->retransmitted_packets_sent.is_defined()) ? 0 : *(outboundStreamStats->retransmitted_packets_sent.cast_to<const webrtc::RTCStatsMember<uint64_t>>());
				msdi.mosi.header_bytes_sent = (!outboundStreamStats->header_bytes_sent.is_defined()) ? 0 : *(outboundStreamStats->header_bytes_sent.cast_to<const webrtc::RTCStatsMember<uint64_t>>());
				msdi.mosi.retransmitted_bytes_sent = (!outboundStreamStats->retransmitted_bytes_sent.is_defined()) ? 0 : *(outboundStreamStats->retransmitted_bytes_sent.cast_to<const webrtc::RTCStatsMember<uint64_t>>());

				msdi.mosi.target_bitrate = (!outboundStreamStats->target_bitrate.is_defined()) ? 0 : *(outboundStreamStats->target_bitrate.cast_to<const webrtc::RTCStatsMember<double>>());
				msdi.mosi.frames_encoded = (!outboundStreamStats->frames_encoded.is_defined()) ? 0 : *(outboundStreamStats->frames_encoded.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
				msdi.mosi.key_frames_encoded = (!outboundStreamStats->key_frames_encoded.is_defined()) ? 0 : *(outboundStreamStats->key_frames_encoded.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
				msdi.mosi.total_encode_time = (!outboundStreamStats->total_encode_time.is_defined()) ? 0 : *(outboundStreamStats->total_encode_time.cast_to<const webrtc::RTCStatsMember<double>>());
				msdi.mosi.total_encoded_bytes_target = (!outboundStreamStats->total_encoded_bytes_target.is_defined()) ? 0 : *(outboundStreamStats->total_encoded_bytes_target.cast_to<const webrtc::RTCStatsMember<uint64_t>>());

				msdi.mosi.key_frames_encoded = (!outboundStreamStats->key_frames_encoded.is_defined()) ? 0 : *(outboundStreamStats->key_frames_encoded.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
				msdi.mosi.total_encode_time = (!outboundStreamStats->total_encode_time.is_defined()) ? 0 : *(outboundStreamStats->total_encode_time.cast_to<const webrtc::RTCStatsMember<double>>());
				msdi.mosi.total_encoded_bytes_target = (!outboundStreamStats->total_encoded_bytes_target.is_defined()) ? 0 : *(outboundStreamStats->total_encoded_bytes_target.cast_to<const webrtc::RTCStatsMember<uint64_t>>());
				msdi.mosi.frame_width = (!outboundStreamStats->frame_width.is_defined()) ? 0 : *(outboundStreamStats->frame_width.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
				msdi.mosi.frame_height = (!outboundStreamStats->frame_height.is_defined()) ? 0 : *(outboundStreamStats->frame_height.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
				msdi.mosi.frames_per_second = (!outboundStreamStats->frames_per_second.is_defined()) ? 0 : *(outboundStreamStats->frames_per_second.cast_to<const webrtc::RTCStatsMember<double>>());
				msdi.mosi.frames_sent = (!outboundStreamStats->frames_sent.is_defined()) ? 0 : *(outboundStreamStats->frames_sent.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
				msdi.mosi.huge_frames_sent = (!outboundStreamStats->huge_frames_sent.is_defined()) ? 0 : *(outboundStreamStats->huge_frames_sent.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
				msdi.mosi.total_packet_send_delay = (!outboundStreamStats->total_packet_send_delay.is_defined()) ? 0 : *(outboundStreamStats->total_packet_send_delay.cast_to<const webrtc::RTCStatsMember<double>>());

				msdi.mosi.quality_limitation_reason = (!outboundStreamStats->quality_limitation_reason.is_defined()) ? "" : *(outboundStreamStats->quality_limitation_reason.cast_to<const webrtc::RTCStatsMember<std::string>>());
				msdi.mosi.quality_limitation_resolution_changes = (!outboundStreamStats->quality_limitation_resolution_changes.is_defined()) ? 0 : *(outboundStreamStats->quality_limitation_resolution_changes.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
				msdi.mosi.content_type = (!outboundStreamStats->content_type.is_defined()) ? "" : *(outboundStreamStats->content_type.cast_to<const webrtc::RTCStatsMember<std::string>>());
				msdi.mosi.encoder_implementation = (!outboundStreamStats->encoder_implementation.is_defined()) ? "" : *(outboundStreamStats->encoder_implementation.cast_to<const webrtc::RTCStatsMember<std::string>>());

				bool isAudio = media_type=="audio";
				bool isVideo = media_type=="video";
				if(isAudio)
				{	
					msdi.mosi.audio_bytes_sent = (!outboundStreamStats->bytes_sent.is_defined()) ? 0 : *(outboundStreamStats->bytes_sent.cast_to<const webrtc::RTCStatsMember<uint64_t>>());
					if(isEnableLog)
					{
						ELOGD << format_string("[RTCOutboundRTPStreamStats][A] media_type(%s) instance(0x%08p) bytes_sent(%lld) packets_sent(%d) target_bitrate(%lf) frames_encoded(%d)", media_type.c_str(), &outboundStreamStats, msdi.mosi.audio_bytes_sent, msdi.mosi.packets_sent, msdi.mosi.target_bitrate, msdi.mosi.frames_encoded);
					}
				}
				else if(isVideo)
				{	
					msdi.mosi.video_bytes_sent = (!outboundStreamStats->bytes_sent.is_defined()) ? 0 : *(outboundStreamStats->bytes_sent.cast_to<const webrtc::RTCStatsMember<uint64_t>>());

					msdi.vssi.ssrc = (!outboundStreamStats->ssrc.is_defined()) ? 0 : *(outboundStreamStats->ssrc.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
					msdi.vssi.is_remote = (!outboundStreamStats->is_remote.is_defined()) ? 0 : *(outboundStreamStats->is_remote.cast_to<const webrtc::RTCStatsMember<bool>>());
					msdi.vssi.track_id = (!outboundStreamStats->track_id.is_defined()) ? "" : *(outboundStreamStats->track_id.cast_to<const webrtc::RTCStatsMember<std::string>>());
					msdi.vssi.transport_id = (!outboundStreamStats->transport_id.is_defined()) ? "" : *(outboundStreamStats->transport_id.cast_to<const webrtc::RTCStatsMember<std::string>>());
					msdi.vssi.codec_id = (!outboundStreamStats->codec_id.is_defined()) ? "" : *(outboundStreamStats->codec_id.cast_to<const webrtc::RTCStatsMember<std::string>>());

					msdi.vssi.fir_count = (!outboundStreamStats->fir_count.is_defined()) ? 0 : *(outboundStreamStats->fir_count.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
					msdi.vssi.pli_count = (!outboundStreamStats->pli_count.is_defined()) ? 0 : *(outboundStreamStats->pli_count.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
					msdi.vssi.nack_count = (!outboundStreamStats->nack_count.is_defined()) ? 0 : *(outboundStreamStats->nack_count.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
					msdi.vssi.sli_count = (!outboundStreamStats->sli_count.is_defined()) ? 0 : *(outboundStreamStats->sli_count.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
					msdi.vssi.qp_sum = (!outboundStreamStats->qp_sum.is_defined()) ? 0 : *(outboundStreamStats->qp_sum.cast_to<const webrtc::RTCStatsMember<uint64_t>>());
					
					if(isEnableLog)
					{
						ELOGD << format_string("[RTCOutboundRTPStreamStats][V] media_type(%s) instance(0x%08p) media_source_id(%s) remote_id(%s) rid(%s)", media_type.c_str(), &outboundStreamStats, msdi.mosi.media_source_id.c_str(), msdi.mosi.remote_id.c_str(), msdi.mosi.rid.c_str());
						ELOGD << format_string("[RTCOutboundRTPStreamStats][V] media_type(%s) instance(0x%08p) packets_sent(%d) retransmitted_packets_sent(%lld) target_bitrate(%lf) frames_encoded(%d) key_frames_encoded(%d)", media_type.c_str(), &outboundStreamStats, msdi.mosi.packets_sent, msdi.mosi.retransmitted_packets_sent, msdi.mosi.target_bitrate, msdi.mosi.frames_encoded, msdi.mosi.key_frames_encoded);
						ELOGD << format_string("[RTCOutboundRTPStreamStats][V] media_type(%s) instance(0x%08p) bytes_sent(%lld) header_bytes_sent(%lld) retransmitted_bytes_sent(%lld)", media_type.c_str(), &outboundStreamStats, msdi.mosi.video_bytes_sent, msdi.mosi.header_bytes_sent, msdi.mosi.retransmitted_bytes_sent);
						ELOGD << format_string("[RTCOutboundRTPStreamStats][V] media_type(%s) instance(0x%08p) total_encode_time(%lf) total_encoded_bytes_target(%lld) retransmitted_bytes_sent(%lld)", media_type.c_str(), &outboundStreamStats, msdi.mosi.total_encode_time, msdi.mosi.total_encoded_bytes_target);
						ELOGD << format_string("[RTCOutboundRTPStreamStats][V] media_type(%s) instance(0x%08p) resolution(%d %d) frames_per_second(%lf) frames_sent(%d) huge_frames_sent(%d) total_packet_send_delay(%lf)", media_type.c_str(), &outboundStreamStats, msdi.mosi.frame_width, msdi.mosi.frame_height, msdi.mosi.frames_per_second, msdi.mosi.frames_sent, msdi.mosi.huge_frames_sent, msdi.mosi.total_packet_send_delay);
						ELOGD << format_string("[RTCOutboundRTPStreamStats][V] media_type(%s) instance(0x%08p) content_type(%s) encoder_implementation(%s) quality_limitation_resolution_changes(%d) quality_limitation_reason(%s)", media_type.c_str(), &outboundStreamStats, msdi.mosi.content_type.c_str(), msdi.mosi.encoder_implementation.c_str(), msdi.mosi.quality_limitation_resolution_changes, msdi.mosi.quality_limitation_reason.c_str());
						ELOGD << format_string("[RTCRTPStreamStats][V] media_type(%s) instance(0x%08p) is_remote(%d) ssrc(%d) track_id(%s) transport_id(%s) codec_id(%s)", media_type.c_str(), &outboundStreamStats, msdi.vssi.is_remote, msdi.vssi.ssrc, msdi.vssi.track_id.c_str(), msdi.vssi.transport_id.c_str(), msdi.vssi.codec_id.c_str());
						ELOGD << format_string("[RTCRTPStreamStats][V] media_type(%s) instance(0x%08p) fir_count(%d) pli_count(%d) nack_count(%d) sli_count(%d) qp_sum(%lld)", media_type.c_str(), &outboundStreamStats, msdi.vssi.fir_count, msdi.vssi.pli_count, msdi.vssi.nack_count, msdi.vssi.sli_count, msdi.vssi.qp_sum);

						{
							std::string encoder_name = g_es->GetEncoderName();
							uint64_t capture_count = g_es->GetCaptureCount();
							uint64_t encode_count = msdi.mosi.frames_encoded;
							double encode_process_time = msdi.mosi.total_encode_time * 1000;
							double fps = msdi.mosi.frames_per_second;
							//double playtime = (double)g_es->GetPlayTime();
							double playtime = (double)std::chrono::duration_cast<std::chrono::milliseconds>(now-m_start_time).count() / 1000;
							uint64_t sent_bytes = msdi.mosi.audio_bytes_sent + msdi.mosi.video_bytes_sent;
							uint64_t sent_bytes_per_sec = (uint64_t)(sent_bytes/playtime);
							uint32_t key_frames_encoded = (msdi.mosi.key_frames_encoded==0) ? 1 : msdi.mosi.key_frames_encoded;
							
#if USE_ENCODER_PERFORMANCE_TEST
							g_es->SetSentBytes(sent_bytes);
#endif

							if(playtime>0)
							{
								ELOGI << format_string("[EncoderStatistics][%s][AVG__] playtime(%.0lf) capture(%.2lf) encode(%.2lf) diff(%.2lf) encode_time/s(%.2lf) encode_time/f(%.2lf)", encoder_name.c_str(), playtime, (double)capture_count/playtime, (double)encode_count/playtime, (double)(capture_count-encode_count)/playtime, (double)encode_process_time/playtime, (double)encode_process_time/encode_count);
								ELOGI << format_string("[EncoderStatistics][%s][TOTAL] playtime(%.0lf) capture(%lld) encode(%lld) diff(%lld) encode_time(%.2lf)", encoder_name.c_str(), playtime, capture_count, encode_count, capture_count-encode_count, encode_process_time);
								ELOGI << format_string("[EncoderStatistics][%s][STAT_] playtime(%.0lf) sent_bytes(%lld) sent_bytes/s(%lldKbps)", encoder_name.c_str(), playtime, sent_bytes*8/1000, sent_bytes_per_sec*8/1000);
								ELOGI << format_string("[EncoderStatistics][%s][RATE_] playtime(%.0lf) droprate(%03.2lf%%) dropcount/s(%.2lf) encodetimerate(%03.2lf%%)", encoder_name.c_str(), playtime, (double)(capture_count-encode_count)/capture_count, (double)(capture_count-encode_count)/playtime, (double)(encode_process_time/(playtime*10)));
								ELOGI << format_string("[EncoderStatistics][%s][ETC__] playtime(%.0lf) fps(%.2lf) key(%d) key_interval(%.2lf)", encoder_name.c_str(), playtime, fps, msdi.mosi.key_frames_encoded, playtime/key_frames_encoded);
							}
						}
					}
				}
				else
				{
					if(isEnableLog)
					{
						uint64_t bytes_sent = msdi.mosi.audio_bytes_sent = (!outboundStreamStats->bytes_sent.is_defined()) ? 0 : *(outboundStreamStats->bytes_sent.cast_to<const webrtc::RTCStatsMember<uint64_t>>());
						ELOGD << format_string("[RTCOutboundRTPStreamStats][X] media_type(%s) instance(0x%08p) bytes_sent(%lld) packets_sent(%d) target_bitrate(%lf) frames_encoded(%d)", media_type.c_str(), &outboundStreamStats, bytes_sent, msdi.mosi.packets_sent, msdi.mosi.target_bitrate, msdi.mosi.frames_encoded);
					}
				}
			}

			for (auto& mediaStreamTrackStats : report->GetStatsOfType<webrtc::RTCMediaStreamTrackStats>()) 
			{
				msdi.kind = (!mediaStreamTrackStats->kind.is_defined()) ? 0 : *(mediaStreamTrackStats->kind.cast_to<const webrtc::RTCStatsMember<std::string>>());
				bool isAudio = msdi.kind=="audio";
				bool isVideo = msdi.kind=="video";
				if(isAudio==false && isVideo==false) continue;

				if(isAudio)
				{	
					if(isEnableLog)
					{
						//ELOGD << format_string("[RTCMediaStreamTrackStats][A] kind(%s) instance(0x%08p) audio_bytes_sent(%lld) packets_sent(%d) target_bitrate(%lf) frames_encoded(%d)", msdi.kind.c_str(), &mediaStreamTrackStats, msdi.audio_bytes_sent, msdi.packets_sent, msdi.target_bitrate, msdi.frames_encoded);
					}
				}
				else if(isVideo)
				{
					msdi.vstsi.frame_width = (!mediaStreamTrackStats->frame_width.is_defined()) ? 0 : *(mediaStreamTrackStats->frame_width.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
					msdi.vstsi.frame_height = (!mediaStreamTrackStats->frame_height.is_defined()) ? 0 : *(mediaStreamTrackStats->frame_height.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
					msdi.vstsi.frames_sent = (!mediaStreamTrackStats->frames_sent.is_defined()) ? 0 : *(mediaStreamTrackStats->frames_sent.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
					msdi.vstsi.huge_frames_sent = (!mediaStreamTrackStats->huge_frames_sent.is_defined()) ? 0 : *(mediaStreamTrackStats->huge_frames_sent.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
					msdi.vstsi.frames_received = (!mediaStreamTrackStats->frames_received.is_defined()) ? 0 : *(mediaStreamTrackStats->frames_received.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
					msdi.vstsi.frames_decoded = (!mediaStreamTrackStats->frames_decoded.is_defined()) ? 0 : *(mediaStreamTrackStats->frames_decoded.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
					msdi.vstsi.frames_dropped = (!mediaStreamTrackStats->frames_dropped.is_defined()) ? 0 : *(mediaStreamTrackStats->frames_dropped.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
					msdi.vstsi.frames_corrupted = (!mediaStreamTrackStats->frames_corrupted.is_defined()) ? 0 : *(mediaStreamTrackStats->frames_corrupted.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
					msdi.vstsi.partial_frames_lost = (!mediaStreamTrackStats->partial_frames_lost.is_defined()) ? 0 : *(mediaStreamTrackStats->partial_frames_lost.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
					msdi.vstsi.full_frames_lost = (!mediaStreamTrackStats->full_frames_lost.is_defined()) ? 0 : *(mediaStreamTrackStats->full_frames_lost.cast_to<const webrtc::RTCStatsMember<uint32_t>>());
					msdi.vstsi.frames_per_second = (!mediaStreamTrackStats->frames_per_second.is_defined()) ? 0 : *(mediaStreamTrackStats->frames_per_second.cast_to<const webrtc::RTCStatsMember<double>>());
					msdi.vstsi.jitter_buffer_delay = (!mediaStreamTrackStats->jitter_buffer_delay.is_defined()) ? 0 : *(mediaStreamTrackStats->jitter_buffer_delay.cast_to<const webrtc::RTCStatsMember<double>>());

					msdi.vstsi.freeze_count = (!mediaStreamTrackStats->jitter_buffer_delay.is_defined()) ? 0 : *(mediaStreamTrackStats->jitter_buffer_delay.cast_to<const webrtc::RTCNonStandardStatsMember<uint32_t>>());
					msdi.vstsi.pause_count = (!mediaStreamTrackStats->pause_count.is_defined()) ? 0 : *(mediaStreamTrackStats->pause_count.cast_to<const webrtc::RTCNonStandardStatsMember<uint32_t>>());
					msdi.vstsi.total_freezes_duration = (!mediaStreamTrackStats->total_freezes_duration.is_defined()) ? 0 : *(mediaStreamTrackStats->total_freezes_duration.cast_to<const webrtc::RTCNonStandardStatsMember<double>>());
					msdi.vstsi.total_pauses_duration = (!mediaStreamTrackStats->total_pauses_duration.is_defined()) ? 0 : *(mediaStreamTrackStats->total_pauses_duration.cast_to<const webrtc::RTCNonStandardStatsMember<double>>());
					msdi.vstsi.total_frames_duration = (!mediaStreamTrackStats->total_frames_duration.is_defined()) ? 0 : *(mediaStreamTrackStats->total_frames_duration.cast_to<const webrtc::RTCNonStandardStatsMember<double>>());
					msdi.vstsi.sum_squared_frame_durations = (!mediaStreamTrackStats->sum_squared_frame_durations.is_defined()) ? 0 : *(mediaStreamTrackStats->sum_squared_frame_durations.cast_to<const webrtc::RTCNonStandardStatsMember<double>>());

					if(isEnableLog)
					{
						ELOGD << format_string("[RTCMediaStreamTrackStats][V] kind(%s) instance(0x%08p) frame_size(%dx%d) fps(%f) frame_sent(%d) huge_frames_sent(%d) received(%d) decoded(%d) dropped(%d) corrupted(%d)", msdi.kind.c_str(), &mediaStreamTrackStats, msdi.vstsi.frame_width, msdi.vstsi.frame_height, msdi.vstsi.frames_per_second, msdi.vstsi.frames_sent, msdi.vstsi.huge_frames_sent, msdi.vstsi.frames_received, msdi.vstsi.frames_decoded, msdi.vstsi.frames_dropped, msdi.vstsi.frames_corrupted);
						ELOGD << format_string("[RTCMediaStreamTrackStats][V] kind(%s) instance(0x%08p) partial_frames_lost(%d) full_frames_lost(%d) jitter_buffer_delay(%lf)", msdi.kind.c_str(), &mediaStreamTrackStats, msdi.vstsi.partial_frames_lost, msdi.vstsi.full_frames_lost, msdi.vstsi.jitter_buffer_delay);
						ELOGD << format_string("[RTCMediaStreamTrackStats][V] kind(%s) instance(0x%08p) freeze_count(%d) freeze_count(%d) total_freezes_duration(%lf) total_pauses_duration(%lf) total_frames_duration(%lf) sum_squared_frame_durations(%lf)", msdi.kind.c_str(), &mediaStreamTrackStats, msdi.vstsi.freeze_count, msdi.vstsi.pause_count, msdi.vstsi.total_freezes_duration, msdi.vstsi.total_pauses_duration, msdi.vstsi.total_frames_duration, msdi.vstsi.sum_squared_frame_durations);
					}
				}
			}

			if(isEnableLog)
				ELOGD << format_string("=============================================================================================================");
		}

		m_pNotifier->OnStats(m_peerID, &msi, &msdi);
	}
};
