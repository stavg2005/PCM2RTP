#include "rtpbuilder/PCMReceiver.hpp"
#include <cstdint>
#include <cstring>  


PCMReceiver::PCMReceiver(net::io_context& io, uint16_t localPort)
    : socket_(io, udp::endpoint(udp::v4(), localPort)),
      reservoir_(RES_CAP)                     // allocate once
{}



void PCMReceiver::start()
{
    if (!receiving_) {
        receiving_ = true;
        doReceive();
    }
}



void PCMReceiver::consumeFrame()
{
    std::memmove(reservoir_.data(),
                 reservoir_.data() + FRAME_SIZE_BYTES,
                 size_ - FRAME_SIZE_BYTES);
    size_ -= FRAME_SIZE_BYTES;
}



void PCMReceiver::ensureSpace(size_t need)
{
    // ensure at least `need` bytes of free tail space
    if (reservoir_.size() - size_ < need) {
        // shift unread bytes to the front
        std::memmove(reservoir_.data(),
                     reservoir_.data() + size_,    
                     size_);
    
    }
}



void PCMReceiver::doReceive()
{
    ensureSpace(MAX_UDP_PACKET);                    // make room if needed
    socket_.async_receive_from(
        net::buffer(reservoir_.data() + size_,
                    reservoir_.size() - size_),// tail free space
        senderEndpoint_,
        [this](auto ec, std::size_t n)
        {
            if (ec) {
                if (handler_) handler_(ec, {});
                return;
            }

            size_ += n; // grew in place

            while (hasFrame()) {
                if (handler_)
                    handler_({}, peekFrame());
                consumeFrame();
            }

            doReceive();// loop
        });
}
