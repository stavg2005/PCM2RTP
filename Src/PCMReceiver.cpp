
#include "../include/PCMReciver.hpp"
#include <cstdint>
#include <stdint.h>


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

bool PCMReceiver::hasFrame() {
  
  return sbuf_.size() >= FRAME_SIZE_BYTES;
}

boost::span<const uint8_t> PCMReceiver::peekFrame() {
  auto bufs = sbuf_.data();

  auto it = boost::asio::buffers_begin(bufs);

  // buffers_begin gives you a char-based iterator, so reinterpret_cast
  const uint8_t *ptr = reinterpret_cast<const uint8_t *>(&*it);

  boost::span<const uint8_t> frame(ptr, FRAME_SIZE_BYTES);

  return frame;
}

void PCMReceiver::consumeFrame() { sbuf_.consume(FRAME_SIZE_BYTES); }
// Extracts one frame from the buffer

void PCMReceiver::doReceive() {

  auto bufs =
      sbuf_.prepare(2048); // Prepare space for whatever datagram comes in
  socket_.async_receive_from(bufs, senderEndpoint_,
                             [this](auto ec, std::size_t n) {
                               if (ec) {
                                 pendingHandler_(ec, {});
                                 return;
                               }

                               sbuf_.commit(n); // make those n bytes readable

                               while (hasFrame()) {
                                 // give the handler the span view
                                 pendingHandler_({}, peekFrame());
                          
                               }
                               // no full frame yet → keep listening for more
                               // UDP
                               doReceive();
                             });
}
