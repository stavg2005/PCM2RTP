
#include "../include/PCMReciver.hpp"
#include "boost/asio/buffer.hpp"
#include <cstdint>
#include <stdint.h>
#include <vector>

PCMReceiver::PCMReceiver(net::io_context &io, uint16_t localPort)
    : socket_(io, udp::endpoint(
                      udp::v4(),
                      localPort)) // runs socket on localhost and a given port
{}

void PCMReceiver::GetNextFrameasync(FrameHandler handler) {
  // Store the user's handler so specify what to do after reciving data
  pendingHandler_ = std::move(handler);
  // Start (or continue) receiving datagrams
  doReceive();
}

bool PCMReceiver::hasFrame() { return sbuf_.size() >= FRAME_SIZE_BYTES; }

// Extracts one frame from the buffer
std::vector<uint8_t> PCMReceiver::popFrame() {
  std::vector<uint8_t> frame(FRAME_SIZE_BYTES);
  boost::asio::buffer_copy(boost::asio::buffer(frame), sbuf_.data());
  sbuf_.consume(FRAME_SIZE_BYTES);
  return frame;
}

void PCMReceiver::doReceive() {

  auto bufs = sbuf_.prepare(2048);
  socket_.async_receive_from(bufs, senderEndpoint_,
                             [this](auto ec, std::size_t n) {
                               if (ec) {
                                 pendingHandler_(ec, {});
                                 return;
                               }
                               sbuf_.commit(n);

                               if (hasFrame()) {
                                 pendingHandler_({}, popFrame());
                               } else {
                                 // If not even one frame was received, wait for
                                 // the next packet.
                                 doReceive();
                               }
                             });
}
