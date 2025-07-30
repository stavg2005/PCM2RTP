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
        
                auto frame = peekFrame();
                pendingHandler_({}, frame);
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

// Returns a read-only view (span) over the current frame’s bytes without copying them
boost::span<const uint8_t> PCMReceiver::peekFrame() {

    // Get the sequence of memory blocks (const_buffer objects) currently stored in the streambuf
    auto bufs = sbuf_.data();

    // Create an iterator to the first byte of the combined data sequence (treats all blocks as one continuous array)
    auto it = boost::asio::buffers_begin(bufs);

    // Take the address of the first byte and reinterpret it as a pointer to uint8_t
    const uint8_t *ptr = reinterpret_cast<const uint8_t *>(&*it);

    // Return a span (read-only view) of FRAME_SIZE_BYTES starting at ptr (no copying)
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
  
    
    // sbuf_.prepare() returns a `mutable_buffers` object – a sequence of writable memory chunks.
    // Each chunk is a boost::asio::mutable_buffer (pointer + size).
    // Boost.Asio will fill these chunks with incoming data without copying them into a temporary array.
    // bufs is now a list of those buffers that are in the new allocated area
    auto bufs = sbuf_.prepare(2048); 

    //Start an asynchronous receive on the UDP socket.
    //When data arrives, Boost.Asio will write into bufs and invoke the lambda callback.
    socket_.async_receive_from(bufs, senderEndpoint_,
        
       
        [this](boost::system::error_code ec, std::size_t bytesReceived) {
             
            if (ec) {
                std::cerr << "UDP receive error: " << ec.message() << std::endl;
                receiving_ = false;
                if (pendingHandler_) {
                    pendingHandler_(ec, {}); // Invoke handler with the error and an empty frame
                }
                return; // Exit early – don’t continue the receive loop
            }

            //    Mark the received bytes as “ready for reading” in the streambuf.
            //    Before commit(), the bytes are just reserved (put area); 
            //    commit() moves them into the readable area (get area).
            sbuf_.commit(bytesReceived);
            
            //  If there’s enough data to form a complete frame
            if (hasFrame()) {   
                //   Get a span (read-only view) of the complete frame without copying memory.
                auto frame = peekFrame();
                
                //   Call the handler: no error ({}), and pass the frame.
                pendingHandler_({}, frame);
                
                //    Immediately start another async receive — 
                doReceive();
            }else{
              //  If we don’t yet have a full frame, keep receiving more data.
                doReceive();
            }

             
            
        });
}