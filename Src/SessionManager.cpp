
#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <rtpbuilder/PCMReceiver.hpp>
#include <rtpbuilder/SessionManager.hpp>
#include <rtpbuilder/alaw.hpp>
#include <rtpbuilder/packet.hpp>

using namespace std::chrono_literals;

namespace {
constexpr std::chrono::milliseconds FRAME_PERIOD{PCMReceiver::MS};
constexpr std::size_t MAX_QUEUE = 100;
} // namespace

//---------------------------------------------------------------------
SessionManager::SessionManager(boost::asio::io_context &io, uint16_t localPort,
                               const std::string &remoteAddr,
                               uint16_t remotePort)
    : receiver_(io, localPort), packetizer_(PCMReceiver::PT, generate_ssrc(),
                                            PCMReceiver::SAMPLES_PER_FRAME),
      transmitter_(io, remoteAddr, remotePort), sendTimer_(io) {
  std::cout << "[Session] listen " << localPort << " → " << remoteAddr << ':'
            << remotePort << std::endl;
}

void SessionManager::start() {
  // Receiver callback: push each full 20 ms PCM frame ─────────
  receiver_.setFrameHandler([this](const boost::system::error_code &ec,
                                   boost::span<const uint8_t> frame) {
    if (ec) {
      std::cerr << "[RX] error: " << ec.message() << std::endl;
      return;
    }
    if (frameQueue_.size() < MAX_QUEUE)
      frameQueue_.emplace(frame.begin(), frame.end());
    else
      std::cerr << "[RX] drop (queue full)\n";
  });
  receiver_.start();

  //  Kick first tick exactly 20 ms from now
  nextTick_ = std::chrono::steady_clock::now() + FRAME_PERIOD;
  schedule();
}

void SessionManager::schedule() {
  sendTimer_.expires_at(nextTick_);
  sendTimer_.async_wait(
      [self = shared_from_this()](auto ec) { self->onTick(ec); });
}

void SessionManager::onTick(const boost::system::error_code &ec) {
  if (ec)
    return;

  auto now = std::chrono::steady_clock::now();

  // Determine how many 20 ms slots have elapsed (>=1 always)
  auto overdue = now - nextTick_;
  std::size_t slots = 1 + overdue / FRAME_PERIOD; // catch‑up count

  // Ship slots frames (or as many as we have queued)
  for (std::size_t i = 0; i < slots && !frameQueue_.empty(); ++i) {
    auto pcm = std::move(frameQueue_.front());
    frameQueue_.pop();

    const int16_t *s16 = reinterpret_cast<const int16_t *>(pcm.data());
    const auto samples_span = boost::span<const int16_t>(
        reinterpret_cast<const int16_t *>(pcm.data()),
        pcm.size() / sizeof(int16_t));
    std::vector<int16_t> samples(samples_span.begin(), samples_span.end());
    std::vector<uint8_t> alaw = encode_alaw(samples);

   rtpScratch_.resize(12 + alaw.size());               // just adjusts .size()
    packetizer_.packetize(alaw, false,
                      {rtpScratch_.data(), rtpScratch_.size()});
transmitter_.enqueue(std::move(rtpScratch_));   
  }

  // Move schedule forward and arm the next wait
  nextTick_ += FRAME_PERIOD * slots;
  schedule();
}
