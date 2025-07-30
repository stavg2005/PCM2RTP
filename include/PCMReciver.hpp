#pragma once
#include "boost/asio/streambuf.hpp"
#include <boost/asio.hpp>
#include <boost/core/span.hpp>
#include <cstdint>
#include <functional>
#include <vector>

namespace net = boost::asio;
using udp = net::ip::udp;
class PCMReceiver {
public:
  // user changeable setting depending on the format the data is sent
  static constexpr unsigned SR = 8000; // Hz
  static constexpr unsigned CH = 1;
  static constexpr unsigned BPS = 16; // bits per sample (PCM)
  static constexpr unsigned MS = 20;  // ms per frame

  static constexpr unsigned PT = 8; // Payload type 8 for G.711 Alaw

  static constexpr size_t SAMPLES_PER_FRAME = SR * MS / 1000;
  // mulitply samples of frame by channels because each channel  gives  another
  // sample
  //  and then multiply by (bps/8) to get the amount of bytes that is sample
  //  because each byte has 8 bits (16/8) =2 bytes
  static constexpr size_t FRAME_SIZE_BYTES = SAMPLES_PER_FRAME * CH * (BPS / 8);

  // cheak for error and then pass the RTPTransmitter
  using FrameHandler = std::function<void(const boost::system::error_code &ec,
                                          boost::span<const uint8_t>)>;

  PCMReceiver(boost::asio::io_context &io, uint16_t localPort);

  bool hasFrame();

  boost::span<const uint8_t> peekFrame();
  // Asynchronously fetch exactly one frame’s worth of PCM.
  void consumeFrame();
  // Invokes handler(ec, frame) when ready (or on error).
  void GetNextFrameasync(FrameHandler handler);
  void doReceive();

  void startContinuousReceiving();

private:
  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint senderEndpoint_;
    
  boost::asio::streambuf sbuf_; // - streambuf manages an internal memory “pool” divided into small blocks.
  FrameHandler pendingHandler_; //       - These blocks have a “put area” (where new data will be written)
  bool receiving_ = false;      //         and a “get area” (where you can read data you’ve already committed).
                                //    📌 Why use streambuf:
                                //       - You don’t need to manually allocate or resize memory for every packet.
                                //       - It can expand automatically and keep partially received data safely
                                //         until you have a full frame.
  
};
