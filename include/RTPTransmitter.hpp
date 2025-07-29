#pragma once
#include <boost/asio.hpp>
#include <boost/core/span.hpp>
#include <cstdint>
#include <functional>
#include <vector>
class RTPTransmitter {
public:
  // cheak for error and call for recive frame to the loop going
  using SendHandler = std::function<void(const boost::system::error_code &ec,
                                         std::size_t bytesSent)>;

  RTPTransmitter(boost::asio::io_context &io, const std::string &remoteAddr,
                 uint16_t remotePort);

  void asyncSend(std::shared_ptr<std::vector<uint8_t>> data, std::size_t size, SendHandler handler);

private:
  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint remoteEndpoint_;
};
