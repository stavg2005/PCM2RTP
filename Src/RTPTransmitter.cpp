// RTPTransmitter.cpp
#include "../include/RTPTransmitter.hpp"
#include "boost/core/span.hpp"
#include <cstdint>
#include <memory>

RTPTransmitter::RTPTransmitter(boost::asio::io_context& io,
                               const std::string& remoteAddr,
                               uint16_t remotePort)
    : socket_(io),
      remoteEndpoint_(boost::asio::ip::make_address(remoteAddr), remotePort)
{
    socket_.open(boost::asio::ip::udp::v4()); 
}

void RTPTransmitter::asyncSend(std::shared_ptr<std::vector<uint8_t>> buf, std::size_t size, SendHandler handler) {
    // ✅ This should be truly async - no blocking!
    socket_.async_send_to(
        boost::asio::buffer(*buf, size),  // Only send 'size' bytes, not the whole buffer
        remoteEndpoint_,
        [buf, handler](const boost::system::error_code& ec, std::size_t bytesSent) {
            // Keep buf alive during the async operation
            handler(ec, bytesSent);
        }
    );
    // ✅ Function returns immediately - doesn't wait for send to complete
}