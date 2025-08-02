// RTPTransmitter.cpp
#include "boost/core/span.hpp"
#include <cstdint>
#include <rtpbuilder/RTPTransmitter.hpp>


RTPTransmitter::RTPTransmitter(boost::asio::io_context &io,
                               const std::string &remoteAddr,
                               uint16_t remotePort)
    : socket_(io),
      remoteEndpoint_(boost::asio::ip::make_address(remoteAddr), remotePort) {
  socket_.open(boost::asio::ip::udp::v4());
}

void RTPTransmitter::enqueue(std::vector<uint8_t>&& pkt) {
 queue_.emplace_back(std::move(pkt));       // move, zero copy
    if (sending_) return;
    sending_ = true;
    doSend();
}

void RTPTransmitter::doSend() {
   auto& msg = queue_.front();                // ref to vector in deque

    socket_.async_send_to(
        boost::asio::buffer(msg),              // buffer(msg) = {msg.data(), msg.size()}
        remoteEndpoint_,
        [this](auto ec, std::size_t)
        {
            queue_.pop_front();                // destroys vector after send
            if (!queue_.empty()) doSend();
            else                 sending_ = false;
        });
}
void RTPTransmitter::asyncSend(boost::span<uint8_t> buf, std::size_t size,
                               SendHandler handler) {

  socket_.async_send_to(
      boost::asio::buffer(buf,
                          size), // Only send 'size' bytes, not the whole buffer
      remoteEndpoint_,
      [buf, handler](const boost::system::error_code &ec,
                     std::size_t bytesSent) {
        // Keep buf alive during the async operation
        handler(ec, bytesSent);
      });
}
