#include "SessionManager.hpp"
#include "PCMReciver.hpp"
#include "alaw.hpp"
#include "boost/core/span.hpp"
#include "packet.hpp"
#include <boost/asio.hpp>
#include <chrono>
#include <cstdint>
#include <iostream>


SessionManager::SessionManager(boost::asio::io_context &io, uint16_t localPort,
                               const std::string &remoteAddr,
                               uint16_t remotePort)
    : reciver_(io, localPort),
      packetizer_(PCMReceiver::PT, generate_ssrc(), PCMReceiver::SAMPLES_PER_FRAME),
      trasmitter_(io, remoteAddr, remotePort), rtpBuffer_(1500), frameCount_(0),
      startTime_(std::chrono::steady_clock::now()) {
  std::cout << "SessionManager created - listening on " << localPort
            << ", sending to " << remoteAddr << ":" << remotePort << std::endl;
}

void SessionManager::start() {
  std::cout << "Starting session..." << std::endl;
  requestNextFrame();
}

void SessionManager::requestNextFrame() {
  reciver_.GetNextFrameasync([this](const boost::system::error_code &ec,
                                    boost::span<const uint8_t> frame) {
    if (ec) {
      std::cerr << "Receive error: " << ec.message() << "\n";
      return;
    }

    
    

    // Process the frame (measure timing)
    packetizeAsync(frame);

    
    requestNextFrame();
  });
}

void SessionManager::packetizeAsync(boost::span<const uint8_t> frame) {

  // Convert to samples FIRST
  const int16_t *samplePtr = reinterpret_cast<const int16_t *>(frame.data());
  std::size_t numSamples = frame.size() / sizeof(int16_t);
  std::vector<int16_t> samples(samplePtr, samplePtr + numSamples);

  // NOW consume frame after we've copied the data
  reciver_.consumeFrame();

  // encode A-law
  std::vector<uint8_t> encoded = encode_alaw(samples);

  boost::span<uint8_t> rtpspan(rtpBuffer_);

  // receive the packet into the rtpspan buffer and get the number of bytes for
  // asyncsend
  int bytes = packetizer_.packetize(encoded, false, rtpBuffer_);
  auto buf = std::make_shared<std::vector<uint8_t>>(rtpBuffer_.begin(),
                                                    rtpBuffer_.begin() + bytes);

  if (bytes == 0) {
    std::cerr << "Error: packetizer returned 0 bytes!" << std::endl;
    return;
  }

  
  trasmitter_.asyncSend(
      buf, bytes,
      [](const boost::system::error_code &ec, std::size_t bytesSent) {
        if (ec) {
          std::cerr << "Send error: " << ec.message() << "\n";
        }
      });
}