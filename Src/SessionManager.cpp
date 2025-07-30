#include "SessionManager.hpp"
#include "PCMReciver.hpp"
#include "alaw.hpp"
#include "boost/core/span.hpp"
#include "packet.hpp"
#include <boost/asio.hpp>
#include <cstdint>
#include <iostream>

///        Handles the full RTP audio pipeline: receives raw PCM frames,
///        encodes them (A-law), packetizes into RTP, and sends them
///        asynchronously.
SessionManager::SessionManager(boost::asio::io_context &io, uint16_t localPort,
                               const std::string &remoteAddr,
                               uint16_t remotePort)
    : reciver_(io, localPort), packetizer_(PCMReceiver::PT, generate_ssrc(),
                                           PCMReceiver::SAMPLES_PER_FRAME),
      trasmitter_(io, remoteAddr, remotePort),
      rtpBuffer_(1500) { // Limit before os trys to fragmnet  so we dont have to change
                         // if we change the size of packets
  std::cout << "SessionManager created - listening on " << localPort
            << ", sending to " << remoteAddr << ":" << remotePort << std::endl;
}


void SessionManager::start() {

    frameHandler_ = [this]
        (const boost::system::error_code& ec,
         boost::span<const uint8_t> frame)
    {
        if (ec) {
            std::cerr << "Receive error: " << ec.message() << '\n';
            return;
        }

        packetizeAsync(frame);    
        reciver_.consumeFrame();

        
        reciver_.GetNextFrameasync(frameHandler_);
    };

    reciver_.GetNextFrameasync(frameHandler_);
}

void SessionManager::packetizeAsync(boost::span<const uint8_t> frame) {

  //  Convert frame bytes into 16-bit signed samples
  const int16_t *samplePtr = reinterpret_cast<const int16_t *>(frame.data());
  std::size_t numSamples = frame.size() / sizeof(int16_t);
  std::vector<int16_t> samples(
      samplePtr, samplePtr + numSamples); // Copy samples for processing


  //  Encode to A-law
  std::vector<uint8_t> encoded = encode_alaw(samples);

  // Packetize into RTP
  int bytes = packetizer_.packetize(encoded, false, rtpBuffer_);
  boost::span<uint8_t> rtpspan(
      rtpBuffer_); // Use span to capture exactly just the frame from the buffer
                   // to avoid sending the entire buffer

  
  if (bytes == 0) {
    std::cerr << "Error: packetizer returned 0 bytes!" << std::endl;
    return;
  }

  
  trasmitter_.asyncSend(
      rtpspan, bytes,
      [](const boost::system::error_code &ec, std::size_t bytesSent) {
        if (ec) {
          std::cerr << "Send error: " << ec.message() << "\n";
        }
        
      });
}
