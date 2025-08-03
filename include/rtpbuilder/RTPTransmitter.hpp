#pragma once
#include <boost/asio.hpp>
#include <boost/core/span.hpp>
#include <cstdint>
#include <functional>
class RTPTransmitter {
public:
  // cheak for error and call for recive frame to the loop going
  using SendHandler = std::function<void(const boost::system::error_code &ec,
                                         std::size_t bytesSent)>;

  RTPTransmitter(boost::asio::io_context &io, const std::string &remoteAddr,
                 uint16_t remotePort);

  void asyncSend(boost::span<uint8_t> data, std::size_t size);

private:
  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint remoteEndpoint_;
};