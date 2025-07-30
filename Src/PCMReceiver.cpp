// PCMReceiver.cpp
#include "../include/PCMReciver.hpp"

#include <boost/asio.hpp>
#include <iostream>
#include <cstdint>




PCMReceiver::PCMReceiver(net::io_context& io, uint16_t localPort)
    : socket_(io, udp::endpoint(udp::v4(), localPort)),
      receiving_(false)
{
    std::cout << "PCMReceiver created on port " << localPort << std::endl;
}

void PCMReceiver::GetNextFrameasync(FrameHandler handler)
{
    // Store the user's handler
    pendingHandler_ = std::move(handler);

    // Start receiving if not already receiving
    if (!receiving_) {
        startContinuousReceiving();
    }
}


bool PCMReceiver::hasFrame()
{
    return sbuf_.size() >= FRAME_SIZE_BYTES;
}

// Returns a read‑only view (span) over the current frame’s bytes without copying them
boost::span<const uint8_t> PCMReceiver::peekFrame()
{
    // Get the sequence of memory blocks (const_buffer objects) currently stored in the streambuf
    auto bufs = sbuf_.data();

    // Create an iterator to the first byte of the combined data sequence
    // (treats all blocks as one continuous array)
    auto it = boost::asio::buffers_begin(bufs);

    // Take the address of the first byte and reinterpret it as a pointer to uint8_t
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&*it);

    // Return a span (read‑only view) of FRAME_SIZE_BYTES starting at ptr (no copying)
    return {ptr, FRAME_SIZE_BYTES};
}

void PCMReceiver::consumeFrame()
{
    // Mark the bytes as consumed so streambuf can reuse that space
    sbuf_.consume(FRAME_SIZE_BYTES);
}



void PCMReceiver::startContinuousReceiving()
{
    receiving_ = true;
    doReceive();
}

void PCMReceiver::doReceive()
{
    /* sbuf_.prepare() returns a `mutable_buffers` object – a sequence of
       writable memory chunks. Each chunk is a boost::asio::mutable_buffer
       (pointer + size). Boost.Asio will fill these chunks with incoming data
       without copying them into a temporary array. `bufs` is now a list of
       those buffers that are in the newly‑allocated area. */
    auto bufs = sbuf_.prepare(2048);

    socket_.async_receive_from(
        bufs, senderEndpoint_,
        [this](boost::system::error_code ec, std::size_t bytesReceived)
        {
            if (ec) {
                std::cerr << "UDP receive error: " << ec.message() << std::endl;
                receiving_ = false;
                if (pendingHandler_) {
                    pendingHandler_(ec, {}); 
                }
                return; 
            }

            // Mark the received bytes as “ready for reading” in the streambuf.
            sbuf_.commit(bytesReceived);

            // While there’s enough data to form a complete frame,deliver every frame immediately 
            while (hasFrame()) {

                // Get a span (read‑only view) of the complete frame without copying memory
                auto frame = peekFrame();

                pendingHandler_({}, frame);

            }

            // Keep receiving more data
            doReceive();
        });
}
