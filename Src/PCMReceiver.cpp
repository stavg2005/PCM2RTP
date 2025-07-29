#include "../include/PCMReciver.hpp"
#include <cstdint>
#include <stdint.h>
#include <iostream>

PCMReceiver::PCMReceiver(net::io_context &io, uint16_t localPort)
    : socket_(io, udp::endpoint(udp::v4(), localPort)),
      receiving_(false)
{
    std::cout << "PCMReceiver created on port " << localPort << std::endl;
}

void PCMReceiver::GetNextFrameasync(FrameHandler handler) {
    // Store the user's handler
    pendingHandler_ = std::move(handler);
    
    // If we already have a complete frame, deliver it immediately
    if (hasFrame()) {
        // Post the handler call to avoid deep recursion
        boost::asio::post(socket_.get_executor(), [this]() {
            if (pendingHandler_ && hasFrame()) {
                auto frame = peekFrame();
                pendingHandler_({}, frame);
            }
        });
        return;
    }
    
    // Start receiving if not already receiving
    if (!receiving_) {
        startContinuousReceiving();
    }
}

bool PCMReceiver::hasFrame() {
    return sbuf_.size() >= FRAME_SIZE_BYTES;
}

boost::span<const uint8_t> PCMReceiver::peekFrame() {
    auto bufs = sbuf_.data();
    auto it = boost::asio::buffers_begin(bufs);
    const uint8_t *ptr = reinterpret_cast<const uint8_t *>(&*it);
    return boost::span<const uint8_t>(ptr, FRAME_SIZE_BYTES);
}

void PCMReceiver::consumeFrame() { 
    sbuf_.consume(FRAME_SIZE_BYTES); 
}

void PCMReceiver::startContinuousReceiving() {
    receiving_ = true;
    doReceive();
}

void PCMReceiver::doReceive() {
    auto bufs = sbuf_.prepare(2048); // Prepare space for incoming datagram
    
    socket_.async_receive_from(bufs, senderEndpoint_,
        [this](boost::system::error_code ec, std::size_t bytesReceived) {
            if (ec) {
                std::cerr << "UDP receive error: " << ec.message() << std::endl;
                receiving_ = false;
                if (pendingHandler_) {
                    pendingHandler_(ec, {});
                }
                return;
            }

            // Make the received bytes available for reading
            sbuf_.commit(bytesReceived);
            
            // Notify handler if we have a complete frame
            if (hasFrame() && pendingHandler_) {
                auto handler = std::move(pendingHandler_); // Clear the handler
                auto frame = peekFrame();
                
                // Call the handler
                handler({}, frame);
                
                // Continue receiving immediately (don't wait for handler to finish)
                doReceive();
            } else {
                // No complete frame yet, keep receiving
                doReceive();
            }
        });
}