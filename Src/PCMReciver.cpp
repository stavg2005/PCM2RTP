
#include <array>
#include <cstdint>
#include <memory>
#include <stdint.h>
#include <vector>
#include "../include/PCMReciver.hpp"

PCMReceiver::PCMReceiver(net::io_context& io, uint16_t localPort)
 : socket_(io, udp::endpoint(udp::v4(), localPort))
{
  accum_.reserve(frameSizeBytes * 2); // allow some headroom
}

void PCMReceiver::GetNextFrameasync(FrameHandler handler) {
  // Store the user's handler
  pendingHandler_ = std::move(handler);
  // Start (or continue) receiving datagrams
  doReceive();
}

void PCMReceiver::doReceive(){
  auto buf = std::make_shared<std::array<uint8_t, 2048>>();

  socket_.async_receive_from(boost::asio::buffer(*buf),
  senderEndpoint_,[this,buf](auto ec,std::size_t bytes){
      if(ec){
        pendingHandler_(ec ,{});
        return;
      }
      //insert at the end the  amount of bytes recived
      accum_.insert(accum_.end(),buf->data(),buf->data()+bytes);
      
      if(accum_.size()< frameSizeBytes){
        doReceive();
        return;
      }
        
      std::vector<uint8_t> frame(accum_.begin(),accum_.begin()+frameSizeBytes);

      accum_.erase(accum_.begin(),
                   accum_.begin() + frameSizeBytes);

      // Deliver the frame
      pendingHandler_({}, std::move(frame));
  });
}

    static constexpr size_t frameSizeBytes = 960 * 2;

