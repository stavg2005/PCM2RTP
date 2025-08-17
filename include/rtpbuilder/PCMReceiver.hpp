#pragma once
#include <boost/asio.hpp>
#include <filesystem>
#include <string>
#include <memory>
#include <vector>
#include "RTPPacketizer.hpp"
#include "/RTPTransmitter.hpp"
#include "PacketUtils.hpp"
#include <fstream>
namespace net = boost::asio;
using udp = net::ip::udp;

class PCMReceiver {
public:
    PCMReceiver(net::io_context& io, uint16_t local_port, const std::string& remote_ip, uint16_t remote_port);
    PCMReceiver(net::io_context& io, udp::socket&& existing_socket, const std::string& remote_ip, uint16_t remote_port);
    void start();
    void stop();
    void read_pcm_from_wav(const std::filesystem::path& path, const std::string& file_name);
    uint16_t get_local_port() const;
private:
    void readAndEncodeNextFrame();
    void scheduleNextSend();
    void sendPreparedFrame();
    void doReceive();

    static constexpr size_t PCM_SIZE = 160; // Samples per frame (20ms at 8kHz)
    static constexpr size_t SAMPLES_PER_FRAME = 160;
    static constexpr size_t FRAME_SIZE_BYTES = PCM_SIZE * 2; // 16-bit PCM
    static constexpr size_t WAV_HEADER_SIZE = 44;
    static constexpr int MS = 20; // Milliseconds per frame

    net::io_context& io_;
    udp::socket socket_;
    udp::endpoint sender_endpoint_;
    RTPPacketizer packetizer_;
    RTPTransmitter transmitter_;
    net::steady_timer timer_;
    std::unique_ptr<std::ifstream> pcm_file_;
    std::vector<uint8_t> buffer_ = std::vector<uint8_t>(PacketUtils::HEADER_SIZE + FRAME_SIZE_BYTES);
    bool has_frame_ = false;
    std::size_t pending_packet_size_ = 0;
    std::chrono::steady_clock::time_point start_time_;
    std::unique_ptr<std::ifstream> pcmFile_; 
    std::chrono::milliseconds duration_ = std::chrono::milliseconds(0);
    std::chrono::steady_clock::time_point next_tick_;
};
