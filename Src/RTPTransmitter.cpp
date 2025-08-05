// RTPTransmitter.cpp
#include "boost/core/span.hpp"
#include <cstdint>
#include <iostream>
#include <rtpbuilder/RTPTransmitter.hpp>

RTPTransmitter::RTPTransmitter(boost::asio::io_context &io,
                               const std::string &remoteAddr,
                               uint16_t remotePort)
    : socket_(io),
      remoteEndpoint_(boost::asio::ip::make_address(remoteAddr), remotePort) {
  socket_.open(boost::asio::ip::udp::v4());
}

void RTPTransmitter::stop() {
  std::cout << "RTPTransmitter stopping...\n";

  if (socket_.is_open()) {
    try {
      socket_.cancel();
    } catch (const std::exception &e) {
      std::cerr << "Error canceling transmitter: " << e.what() << std::endl;
    }
  }
}

void RTPTransmitter::asyncSend(boost::span< const uint8_t> buf, std::size_t size) {

  socket_.async_send_to(
      boost::asio::buffer(buf,
                          size), // Only send 'size' bytes, not the whole buffer
      remoteEndpoint_,
      [buf](const boost::system::error_code &ec, std::size_t bytesSent) {
        if (ec) {
          std::cerr << "Send error: " << ec.message() << "\n";
        }
      });
}