#pragma once
#include <boost/asio.hpp>
#include <vector>
#include <functional>
#include <cstdint>
namespace net = boost::asio;
using     udp = net::ip::udp;
class PCMReceiver {
public:
  using FrameHandler = std::function<
    void(const boost::system::error_code& ec,
         std::vector<uint8_t> frame)>;

  PCMReceiver(boost::asio::io_context& io, uint16_t localPort);

  // Asynchronously fetch exactly one frame’s worth of PCM.
  // Invokes handler(ec, frame) when ready (or on error).
  void GetNextFrameasync(FrameHandler handler);

  static constexpr size_t frameSizeBytes = 960 * 2;

private:
  void doReceive();  // internal continuation

  boost::asio::ip::udp::socket      socket_;
  boost::asio::ip::udp::endpoint    senderEndpoint_;
  std::vector<uint8_t>              accum_;
  FrameHandler                      pendingHandler_;
};
