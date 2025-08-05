#include "rtpbuilder/RTPPacketizer.hpp"
#include <fstream>
#include <iostream>
#include <memory>
#include <rtpbuilder/PCMReceiver.hpp>
#include <rtpbuilder/PacketUtils.hpp>
#include <rtpbuilder/packet.hpp>
#include <boost/asio.hpp>
#include <boost/core/span.hpp>

PCMReceiver::PCMReceiver(net::io_context &io, uint16_t localPort,
                         const std::string &remoteIp, uint16_t remotePort)
    : socket_(io, udp::endpoint(udp::v4(), localPort)),
      packetizer_(/* payloadType */ 8, generate_ssrc(), SAMPLES_PER_FRAME),
      transmitter_(io, remoteIp, remotePort), timer_(io) {
  std::cout << "PCMReceiver created on port " << localPort << std::endl;
}

void PCMReceiver::start() {
  doReceive();
}

void PCMReceiver::stop() {
  std::cout << "PCMReceiver stopping...\n";
  transmitter_.stop();
  if (socket_.is_open()) {
    try {
      socket_.cancel();
    } catch (const std::exception &e) {
      std::cerr << "Error canceling receiver: " << e.what() << std::endl;
    }
  }
}

void PCMReceiver::read_pcm_from_wav(const std::string &filename) {
  pcmFile_ = std::make_unique<std::ifstream>(filename, std::ios::binary);

  if (!pcmFile_->is_open()) {
    std::cerr << "Failed to open PCM WAV file: " << filename << "\n";
    return;
  }

  pcmFile_->seekg(WAV_HEADER_SIZE, std::ios::beg);
  std::cout << "Reading PCM from WAV file: " << filename << "\n";

  nextTick_ = std::chrono::steady_clock::now();
  readAndEncodeNextFrame();
  scheduleNextSend();
}

void PCMReceiver::readAndEncodeNextFrame() {
  if (!pcmFile_ || !pcmFile_->is_open()) return;

  pcmFile_->read(reinterpret_cast<char *>(buffer_.data() + PacketUtils::HEADER_SIZE), FRAME_SIZE_BYTES);
  std::streamsize bytesRead = pcmFile_->gcount();
  if (bytesRead <= 0) {
    hasFrame_ = false;
    return;
  }

  boost::span<const uint8_t> pcmSpan(buffer_.data() + PacketUtils::HEADER_SIZE, bytesRead);
  pendingPacketSize_ = PacketUtils::packet2rtp(pcmSpan, packetizer_, buffer_);
  hasFrame_ = pendingPacketSize_ > 0;
}

void PCMReceiver::scheduleNextSend() {
  nextTick_ += std::chrono::milliseconds(MS);
  timer_.expires_at(nextTick_);
  timer_.async_wait([this](const boost::system::error_code &ec) {
    if (!ec) sendPreparedFrame();
  });
}

void PCMReceiver::sendPreparedFrame() {
  if (hasFrame_) {
    transmitter_.asyncSend(boost::span<const uint8_t>(buffer_.data(), pendingPacketSize_), pendingPacketSize_);
    readAndEncodeNextFrame();
  }
  scheduleNextSend();
}

void PCMReceiver::doReceive() {
  socket_.async_receive_from(
      boost::asio::buffer(buffer_.data() + PacketUtils::HEADER_SIZE, PCM_SIZE * 2),
      senderEndpoint_,
      [this](boost::system::error_code ec, std::size_t bytesReceived) {
        if (ec == boost::asio::error::operation_aborted) {
          std::cout << "Receive cancelled or stopped.\n";
          return;
        }

        if (ec) {
          std::cerr << "Recv error: " << ec.message() << '\n';
          doReceive();
          return;
        }

        boost::span<const uint8_t> pcmData(buffer_.data() + PacketUtils::HEADER_SIZE, bytesReceived);
        std::size_t pktLen = PacketUtils::packet2rtp(pcmData, packetizer_, buffer_);

        if (pktLen > 0) {
          transmitter_.asyncSend(boost::span<const uint8_t>(buffer_.data(), pktLen), pktLen);
        }

        doReceive();
      });
}


