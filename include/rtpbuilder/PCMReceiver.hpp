#pragma once
#include <array>
#include <boost/asio.hpp>
#include <boost/core/span.hpp>
#include <cstddef>
#include <cstdint>
#include <rtpbuilder/PacketUtils.hpp>

#include "RTPPacketizer.hpp"
#include "RTPTransmitter.hpp"

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

  PCMReceiver(net::io_context &io, uint16_t localPort,
              const std::string &remoteIp, uint16_t remotePort);

  void start();
  void stop();
private:
  void doReceive();

  udp::socket socket_;
  udp::endpoint senderEndpoint_;
  static constexpr size_t MAX_UDP_PACKET = 2048;
  std::array<uint8_t, MAX_UDP_PACKET> buffer_{};
  const std::size_t PCM_SIZE = FRAME_SIZE_BYTES;
  RTPPacketizer packetizer_;
  RTPTransmitter transmitter_;
};
