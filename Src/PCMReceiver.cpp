#include "rtpbuilder/RTPPacketizer.hpp"
#include <boost/asio.hpp>
#include <boost/core/span.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include "rtpbuilder/PCMReceiver.hpp"
#include "rtpbuilder/PacketUtils.hpp"
#include "rtpbuilder/packet.hpp"

PCMReceiver::PCMReceiver(net::io_context &io, uint16_t local_port,
                         const std::string &remote_ip, uint16_t remote_port)
    : io_(io), socket_(io, udp::endpoint(udp::v4(), local_port)),
      packetizer_(8, generate_ssrc(), SAMPLES_PER_FRAME),
      transmitter_(io, remote_ip, remote_port), timer_(io) {
  std::cout << "[PCMReceiver] Created on port " << local_port << "\n";
}
PCMReceiver::PCMReceiver(net::io_context &io, udp::socket &&existing_socket,
                         const std::string &remote_ip, uint16_t remote_port)
    : io_(io), socket_(std::move(existing_socket)),
      packetizer_(8, generate_ssrc(), SAMPLES_PER_FRAME),
      transmitter_(io, remote_ip, remote_port), timer_(io) {
  std::cout << "[PCMReceiver] Created with existing socket on port "
            << socket_.local_endpoint().port() << "\n";
}

void PCMReceiver::start() { doReceive(); }

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

void PCMReceiver::read_pcm_from_wav(const std::filesystem::path& path, const std::string& file_name) {
  pcmFile_ = std::make_unique<std::ifstream>(path, std::ios::binary);

  if (!pcmFile_->is_open()) {
    std::cerr << "Failed to open PCM WAV file: " << file_name << "\n";

    return;
  }

  pcmFile_->seekg(WAV_HEADER_SIZE, std::ios::beg);
  std::cout << "Reading PCM from WAV file: " << file_name << "\n";

  next_tick_ = std::chrono::steady_clock::now();
  readAndEncodeNextFrame();
  scheduleNextSend();
}

void PCMReceiver::readAndEncodeNextFrame() {
  if (!pcmFile_ || !pcmFile_->is_open())
    return;

  pcmFile_->read(
      reinterpret_cast<char *>(buffer_.data() + PacketUtils::HEADER_SIZE),
      FRAME_SIZE_BYTES);
  std::streamsize bytesRead = pcmFile_->gcount();
  if (bytesRead <= 0) {
    has_frame_ = false;
    return;
  }

  boost::span<const uint8_t> pcmSpan(buffer_.data() + PacketUtils::HEADER_SIZE,
                                     bytesRead);
  pending_packet_size_= PacketUtils::packet2rtp(pcmSpan, packetizer_, buffer_);
  has_frame_ = pending_packet_size_ > 0;
}

uint16_t PCMReceiver::get_local_port() const{
  return socket_.local_endpoint().port();
}
void PCMReceiver::scheduleNextSend() {
  next_tick_ += std::chrono::milliseconds(MS);
  timer_.expires_at(next_tick_);
  timer_.async_wait([this](const boost::system::error_code &ec) {
    if (!ec)
      sendPreparedFrame();
  });
}

void PCMReceiver::sendPreparedFrame() {
  if (has_frame_) {
    transmitter_.asyncSend(
        boost::span<const uint8_t>(buffer_.data(), pending_packet_size_),
        pending_packet_size_);
    readAndEncodeNextFrame();
  }
  scheduleNextSend();
}

void PCMReceiver::doReceive() {
  socket_.async_receive_from(
      boost::asio::buffer(buffer_.data() + PacketUtils::HEADER_SIZE,
                          PCM_SIZE * 2),
      sender_endpoint_,
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

        boost::span<const uint8_t> pcmData(
            buffer_.data() + PacketUtils::HEADER_SIZE, bytesReceived);
        std::size_t pktLen =
            PacketUtils::packet2rtp(pcmData, packetizer_, buffer_);

        if (pktLen > 0) {
          transmitter_.asyncSend(
              boost::span<const uint8_t>(buffer_.data(), pktLen), pktLen);
        }

        doReceive();
      });
}
