
#include "../include/PCMReciver.hpp"
#include "boost/asio/buffer.hpp"
#include <cstdint>
#include <stdint.h>
#include <vector>

PCMReceiver::PCMReceiver(net::io_context &io, uint16_t localPort)
    : socket_(io, udp::endpoint(udp::v4(), localPort)) {

}

void PCMReceiver::GetNextFrameasync(FrameHandler handler) {
  // Store the user's handler
  pendingHandler_ = std::move(handler);
  // Start (or continue) receiving datagrams
  doReceive();
}

void PCMReceiver::doReceive() {

  // 1) Allocate space for the next incoming datagram:
  auto bufs = sbuf_.prepare(frameSizeBytes);

  // 2) Kick off the async receive into that writable region:
  socket_.async_receive_from(
      bufs, // <-- mutable-buffer-sequence
      senderEndpoint_,
      [this](boost::system::error_code ec, std::size_t bytesReceived) {
        if (ec) {
          pendingHandler_(ec, {});
          return;
        }

        // 3) Make those bytes part of the readable area:
        sbuf_.commit(bytesReceived);

        // 4) If we still don’t have a full “frame” yet, keep reading:
        if (sbuf_.size() < frameSizeBytes) {
          doReceive();
          return;
        }

        // 5) Pull one frame out:
        std::vector<uint8_t> frame(frameSizeBytes);
        boost::asio::buffer_copy(
            boost::asio::buffer(frame), // dest = your vector’s mutable_buffer
            sbuf_.data()                // src  = streambuf’s readable data
        );
        sbuf_.consume(frameSizeBytes);

        // 6) Hand it back:
        pendingHandler_({}, std::move(frame));
      });
}

static constexpr size_t frameSizeBytes = 960 * 2;
