#include "../include/RTPTransmitter.hpp"
#include "boost/asio/io_context.hpp"
#include "boost/asio/ip/address.hpp"
#include <cstdint>
#include <memory>
#include <minwindef.h>
#include <vector>

RTPTransmitter::RTPTransmitter(boost::asio::io_context &io,
                               const std::string &remoteAddr,
                               uint16_t remotePort, uint16_t localPort)
    : socket_(io),
      remoteEndpoint_(boost::asio::ip::make_address(remoteAddr), remotePort) //conver string to ip adress then creates endpoint with the port
      {
  socket_.open(boost::asio::ip::udp::v4());
  socket_.bind({boost::asio::ip::udp::v4(), localPort});
}

    //data is the rtp packet in byte form inorder to send to client
void RTPTransmitter::asyncSend(const uint8_t *data,std::size_t size,SendHandler handler){
    auto buf = std::make_shared<std::vector<uint8_t>>(data,data+size); //to keep buffer alive until the handler is called
    socket_.async_send_to(boost::asio::buffer(*buf),
    remoteEndpoint_,
    [buf,handler](auto ec,std::size_t bytesSent){
        handler(ec,bytesSent);
    });
}