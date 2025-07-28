#pragma once
#include "boost/asio/awaitable.hpp"
#include "boost/asio/streambuf.hpp"
#include <boost/asio.hpp>
#include <cstdint>
#include <functional>
#include <vector>

namespace net = boost::asio;
using udp = net::ip::udp;
class PCMReceiver {
public:
  using FrameHandler = std::function<void(const boost::system::error_code &ec,
                                          std::vector<uint8_t> frame)>;

  PCMReceiver(boost::asio::io_context &io, uint16_t localPort);

  bool hasFrame();

  std::vector<uint8_t> popFrame();
  // Asynchronously fetch exactly one frame’s worth of PCM.
  // Invokes handler(ec, frame) when ready (or on error).
  void GetNextFrameasync(FrameHandler handler);

  boost::asio::awaitable<std::vector<uint8_t>> asyncGetNextFrameAwait();
  // user changeable setting depending on the format the data is sent
  static constexpr unsigned SR = 8000; // Hz
  static constexpr unsigned CH = 1;
  static constexpr unsigned BPS = 16; // bits per sample (PCM)
  static constexpr unsigned MS = 20;  // ms per frame

  static constexpr size_t SAMPLES_PER_FRAME = SR * MS / 1000; // 160
  static constexpr size_t FRAME_SIZE_BYTES =
      SAMPLES_PER_FRAME * CH * (BPS / 8); // 320
private:
  void doReceive(); // internal continuation

  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint senderEndpoint_;
  boost::asio::streambuf sbuf_;
  FrameHandler pendingHandler_;
  std::array<uint8_t, 1500> receiveBuffer_; // ALLOCATED ONCE
  std::vector<uint8_t> frameBuffer_;
};
