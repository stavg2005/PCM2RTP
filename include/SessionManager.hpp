#pragma once
#include "PCMReciver.hpp"
#include "RTPPacketizer.hpp"
#include "RTPTransmitter.hpp"
#include <boost/asio.hpp>
#include <boost/core/span.hpp>
#include <vector>
#include <chrono>

class SessionManager {
public:
    SessionManager(boost::asio::io_context &io, uint16_t localPort,
                   const std::string &remoteAddr, uint16_t remotePort);
    
    void start();

private:
    void requestNextFrame();
    void packetizeAsync(boost::span<const uint8_t> frame);
    
    PCMReceiver reciver_;
    RTPPacketizer packetizer_;
    RTPTransmitter trasmitter_;
    std::vector<uint8_t> rtpBuffer_;
};