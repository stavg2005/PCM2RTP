
#include "../include/PCMReciver.hpp"
#include "boost/asio/buffer.hpp"
#include <cstdint>
#include <stdint.h>
#include <vector>

// user changeable setting depending on the format the data is sent
constexpr unsigned SR = 8000; // Hz
constexpr unsigned CH = 1;
constexpr unsigned BPS = 16; // bits per sample (PCM)
constexpr unsigned MS = 20;  // ms per frame

constexpr size_t SAMPLES_PER_FRAME = SR * MS / 1000;                    // 160
constexpr size_t FRAME_SIZE_BYTES = SAMPLES_PER_FRAME * CH * (BPS / 8); // 320

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

void PCMReceiver::doReceive() {

  // 1) Allocate space for the next incoming datagram:
  auto bufs = sbuf_.prepare(2048); // big enough for any UDP packet
  // buff point the the new writable memory in the sbuf_ and async_receive_from
  // expects a mutable_buffer_type

  // 2) Kick off the async receive into that writable region:
  socket_.async_receive_from(
      bufs, senderEndpoint_, [this](auto ec, std::size_t n) {
        if (ec) {
          pendingHandler_(ec, {});
          return;
        }
        sbuf_.commit(n);

        while (sbuf_.size() >= FRAME_SIZE_BYTES) {
          std::vector<uint8_t> frame(FRAME_SIZE_BYTES);
          boost::asio::buffer_copy(boost::asio::buffer(frame), sbuf_.data());
          sbuf_.consume(FRAME_SIZE_BYTES);
          pendingHandler_({}, std::move(frame)); // deliver EXACT FRAME_SIZE_BYTES
        }
        doReceive();
      });
}
