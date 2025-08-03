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

void RTPTransmitter::asyncSend(boost::span<uint8_t> buf, std::size_t size) {

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