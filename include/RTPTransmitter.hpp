#pragma once
#include <boost/asio.hpp>
#include <cstdint>
#include <functional>

class RTPTransmitter {
public:
  using SendHandler = std::function<
    void(const boost::system::error_code& ec,
         std::size_t bytesSent)>;

  // ctor: you can pass a bound local port (or 0 for ephemeral)
  RTPTransmitter(boost::asio::io_context& io,
                 const std::string& remoteAddr,
                 uint16_t remotePort,
                 uint16_t localPort = 0);

  // Async send: invokes handler when done
  void asyncSend(const uint8_t *data,
                 std::size_t size,
                 SendHandler handler);

private:
  boost::asio::ip::udp::socket      socket_;
  boost::asio::ip::udp::endpoint    remoteEndpoint_;
};
