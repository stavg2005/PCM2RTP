#pragma once
#include "boost/asio/steady_timer.hpp"
#include <boost/asio.hpp>
#include <boost/core/span.hpp>
#include <chrono>
#include <cstdint>
#include <functional>
#include <vector>
#include <deque>
class RTPTransmitter {
public:
  // cheak for error and call for recive frame to the loop going
  using SendHandler = std::function<void(const boost::system::error_code &ec,
                                         std::size_t bytesSent)>;

  RTPTransmitter(boost::asio::io_context &io, const std::string &remoteAddr,
                 uint16_t remotePort);

  void asyncSend(boost::span<uint8_t> data, std::size_t size, SendHandler handler);
  void enqueue(std::vector<uint8_t>&& pkt);
  void doSend();

private:
  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint remoteEndpoint_;
   std::deque<std::vector<uint8_t>> queue_; 
   bool sending_ = false;
};