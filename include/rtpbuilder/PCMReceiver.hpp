#pragma once
#include <boost/asio.hpp>
#include <boost/core/span.hpp>
#include <array>
#include <cstdint>
#include <functional>
#include <vector>

namespace net = boost::asio;
using udp = net::ip::udp;

class PCMReceiver {
public:
    // ── audio format ──────────────────────────────────────────────
    static constexpr unsigned SR  = 8000;   // Hz
    static constexpr unsigned CH  = 1;
    static constexpr unsigned BPS = 16;     // bits / sample
    static constexpr unsigned MS  = 20;     // ms / frame
    static constexpr unsigned PT  = 8;      // RTP payload-type (PCMA)

    static constexpr size_t SAMPLES_PER_FRAME = SR * MS / 1000;          
    static constexpr size_t FRAME_SIZE_BYTES  =
        SAMPLES_PER_FRAME * CH * (BPS / 8);                             
    static constexpr size_t RES_CAP = 10 * FRAME_SIZE_BYTES;            

    using FrameHandler = std::function<
        void(const boost::system::error_code&, boost::span<const uint8_t>)
    >;

    explicit PCMReceiver(net::io_context& io, uint16_t localPort);

    void setFrameHandler(FrameHandler h) { handler_ = std::move(h); }
    void start();

    bool                         hasFrame()   const { return size_ >= FRAME_SIZE_BYTES; }
    boost::span<const uint8_t>   peekFrame()  { return { reservoir_.data(), FRAME_SIZE_BYTES }; }
    void                         consumeFrame();

private:
    void doReceive();                         
    void ensureSpace(size_t need);            

    udp::socket          socket_;
    udp::endpoint        senderEndpoint_;

    static constexpr size_t MAX_UDP_PACKET = 2048;

    // grow-in-place reservoir 
    std::vector<uint8_t> reservoir_;          // capacity = RES_CAP
    size_t               size_      = 0;      // bytes currently stored

    FrameHandler         handler_;
    bool                 receiving_ = false;
};
