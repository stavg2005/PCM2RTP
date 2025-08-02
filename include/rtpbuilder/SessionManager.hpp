// SessionManager.hpp – paced real‑time RTP sender
// -------------------------------------------------
// A lightweight façade that glues together:
//   • PCMReceiver          – pulls raw 8 kHz mono PCM frames from UDP
//   • RTPPacketizer        – wraps each A‑law‑encoded frame in an RTP header
//   • RTPTransmitter       – ships the packet on another UDP socket
// …and paces the flow with a 20‑ms steady timer so packets leave at
//    proper real‑time cadence (no more burst/jitter).
//
// Use shared_ptr + enable_shared_from_this so callers can just
//   auto sess = std::make_shared<SessionManager>(...);
//   sess->start();
// and forget the lifetime.
// -------------------------------------------------

#pragma once

#include <boost/asio.hpp>
#include <queue>
#include <vector>
#include <cstdint>
#include <string>
#include <memory>
#include <chrono>

#include "rtpbuilder/PCMReceiver.hpp" 
#include "rtpbuilder/RTPPacketizer.hpp"
#include "rtpbuilder/RTPTransmitter.hpp"

class SessionManager : public std::enable_shared_from_this<SessionManager>
{
public:
    SessionManager(boost::asio::io_context &io,
                   uint16_t                localPort,
                   const std::string      &remoteAddr,
                   uint16_t                remotePort);

    /// Kick off async reception + paced transmission
    void start();

    /// Graceful stop (optional – simply cancels timer & sockets)
    void stop();
    void scheduleNextTick();
    void onTick(const boost::system::error_code &ec);
    void schedule();
private:
   
    void onSendTick(const boost::system::error_code &ec);  // 20‑ms heartbeat

    PCMReceiver           receiver_;      
    RTPPacketizer         packetizer_;   
    RTPTransmitter        transmitter_;   
    std::vector<uint8_t> rtpScratch_; 
    boost::asio::steady_timer               sendTimer_;
    std::queue<std::vector<uint8_t>>        frameQueue_; ///< pending frames

    std::chrono::steady_clock::time_point  nextTick_;
    
};
