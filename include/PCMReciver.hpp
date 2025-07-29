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

 
  static constexpr size_t SAMPLES_PER_FRAME = SR * MS / 1000; 
  static constexpr size_t FRAME_SIZE_BYTES =
      SAMPLES_PER_FRAME * CH * (BPS / 8);
  //cheak for error and then pass the RTPTransmitter
  using FrameHandler = std::function<void(const boost::system::error_code &ec,
                                          boost::span<const uint8_t>)>;


  PCMReceiver(boost::asio::io_context &io, uint16_t localPort);

  bool hasFrame();

  boost::span<const uint8_t> peekFrame();
  // Asynchronously fetch exactly one frame’s worth of PCM.
  boost::span<const uint8_t> popFrame();
  void consumeFrame();
  // Invokes handler(ec, frame) when ready (or on error).
  void GetNextFrameasync(FrameHandler handler);
  void doReceive();
  
 
private:
 

  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint senderEndpoint_;
  boost::asio::streambuf sbuf_;
  FrameHandler pendingHandler_;
};
