#include "rtpbuilder/RTPPacketizer.hpp"
#include <iostream>
#include <rtpbuilder/PCMReceiver.hpp>
#include <rtpbuilder/PacketUtils.hpp>
#include <rtpbuilder/packet.hpp>

PCMReceiver::PCMReceiver(net::io_context &io, uint16_t localPort,
                         const std::string &remoteIp, uint16_t remotePort)
    : socket_(io, udp::endpoint(udp::v4(), localPort)),
      packetizer_(/* payloadType */ 8, generate_ssrc(), SAMPLES_PER_FRAME),
      transmitter_(io, remoteIp, remotePort) {
  std::cout << "PCMReceiver created on port " << localPort << std::endl;
}

void PCMReceiver::start() { doReceive(); }

void PCMReceiver::stop() {
  std::cout << "PCMReceiver stopping...\n";

  // Stop the internal transmitter first
  transmitter_.stop();

  // Cancel pending receive operations
  if (socket_.is_open()) {
    try {
      socket_.cancel();
    } catch (const std::exception &e) {
      std::cerr << "Error canceling receiver: " << e.what() << std::endl;
    }
  }
}

void PCMReceiver::doReceive() {

  socket_.async_receive_from(
      boost::asio::buffer(buffer_.data() + PacketUtils::HEADER_SIZE,
                          PCM_SIZE*2),
      senderEndpoint_,
      [this](boost::system::error_code ec, std::size_t bytesReceived) {
        if (ec == boost::asio::error::operation_aborted) {
          std::cout << "Receive cancelled or stopped.\n";
          return;
        }

        if (ec) {
          std::cerr << "Recv error: " << ec.message() << '\n';
          doReceive(); // Continue receiving even on error
          return;
        }

        // Create temporary span for the PCM data we've accumulated
        boost::span<const uint8_t> pcmData(
            buffer_.data() + PacketUtils::HEADER_SIZE, bytesReceived);

        // Convert in-place: the A-law output will be smaller and fit
        std::size_t pktLen = PacketUtils::packet2rtp(
            pcmData, // source: PCM after header
            packetizer_,
            {buffer_.data(), buffer_.size()} // dest: from beginning
        );

        if (pktLen == 0) {
          std::cerr << "packetize failed\n";
          doReceive();
          return;
        }

        transmitter_.asyncSend({buffer_.data(), buffer_.size()}, pktLen);

        doReceive();
      });
}
