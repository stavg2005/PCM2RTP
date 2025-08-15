#pragma once
#include "RTPPacketizer.hpp"
#include "RTPTransmitter.hpp"
#include <array>
#include <boost/asio.hpp>
#include <boost/core/span.hpp>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <queue>
#include <rtpbuilder/PacketUtils.hpp>


namespace net = boost::asio;
using udp = net::ip::udp;

class PCMReceiver {
public:
  // ── audio format ──────────────────────────────────────────────
  static constexpr unsigned SR = 8000; // Hz
  static constexpr unsigned CH = 1;
  static constexpr unsigned BPS = 16; // bits / sample
  static constexpr unsigned MS = 20;  // ms / frame
  static constexpr unsigned PT = 8;   // RTP payload-type (PCMA)

  static constexpr size_t SAMPLES_PER_FRAME = SR * MS / 1000;
  static constexpr size_t FRAME_SIZE_BYTES = SAMPLES_PER_FRAME * CH * (BPS / 8);
  static constexpr size_t RES_CAP = 10 * FRAME_SIZE_BYTES;
  static constexpr size_t WAV_HEADER_SIZE = 44;
  static constexpr size_t MAX_FRAMES = 1000;
  PCMReceiver(net::io_context &io, uint16_t localPort,
              const std::string &remoteIp, uint16_t remotePort);

  void start();
  void stop();
  void read_pcm_from_wav(std::filesystem::path path ,const std::string &filename);

private:
  void doReceive();
  void sendNextFrame();
  void scheduleNextSend();
  void sendPreparedFrame();
  void readAndEncodeNextFrame();
  udp::socket socket_;
  udp::endpoint senderEndpoint_;
  static constexpr size_t MAX_UDP_PACKET = 2048;
  std::array<uint8_t, FRAME_SIZE_BYTES * MAX_FRAMES> buffer_{};
  const std::size_t PCM_SIZE = FRAME_SIZE_BYTES;
  std::size_t nextPacketOffset_ = 0;
  net::steady_timer timer_;      
  std::unique_ptr<std::ifstream> pcmFile_; // managed WAV file
  std::chrono::steady_clock::time_point nextTick_;
  bool hasFrame_;
  std::deque<boost::span<const uint8_t>> frameQueue_;
  RTPPacketizer packetizer_;
  RTPTransmitter transmitter_;
  std::size_t pendingPacketSize_ = 0;
};
