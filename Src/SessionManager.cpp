#include "SessionManager.hpp"
#include "PCMReciver.hpp"
#include "alaw.hpp"
#include "boost/core/span.hpp"
#include "packet.hpp"
#include <boost/asio.hpp>
#include <chrono>
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
      rtpBuffer_(1500) { // Typical MTU size for UDP so we dont have to change
                         // if we change the size of packets
  std::cout << "SessionManager created - listening on " << localPort
            << ", sending to " << remoteAddr << ":" << remotePort << std::endl;
}

/// Starts the session: begins the receive loop to continuously get
/// frames.
void SessionManager::start() {
  std::cout << "Starting session..." << std::endl;
  requestNextFrame(); // Begin the async receive loop
}

///  Requests the next audio frame from PCMReceiver asynchronously.
///  When a frame arrives, processes it and schedules the next request.
void SessionManager::requestNextFrame() {
  reciver_.GetNextFrameasync([this](const boost::system::error_code &ec,
                                    boost::span<const uint8_t> frame) {
    if (ec) {
      std::cerr << "Receive error: " << ec.message() << "\n";
      return;
    }

    packetizeAsync(
        frame); // Process the received frame (encode + packetize + send)
    requestNextFrame(); // Immediately schedule the next receive (keeps the
                        // stream alive)
  });
}

///  Converts a raw PCM frame into A-law encoded audio,
///  wraps it in an RTP packet, and sends it asynchronously.
void SessionManager::packetizeAsync(boost::span<const uint8_t> frame) {

  //  Convert frame bytes into 16-bit signed samples ---
  const int16_t *samplePtr = reinterpret_cast<const int16_t *>(frame.data());
  std::size_t numSamples = frame.size() / sizeof(int16_t);
  std::vector<int16_t> samples(
      samplePtr, samplePtr + numSamples); // Copy samples for processing

  // Mark the frame as consumed so streambuf can reuse that space
  reciver_.consumeFrame();

  //  Encode to A-law
  std::vector<uint8_t> encoded = encode_alaw(samples);

  // Packetize into RTP
  int bytes = packetizer_.packetize(encoded, false, rtpBuffer_);
  boost::span<uint8_t> rtpspan(
      rtpBuffer_); // Use span to capture exactly just the frame from the buffer
                   // to avoid sending the entire buffer

  // Check for errors from packetizer
  if (bytes == 0) {
    std::cerr << "Error: packetizer returned 0 bytes!" << std::endl;
    return;
  }

  // Send asynchronously via the transmitter
  trasmitter_.asyncSend(
      rtpspan, bytes,
      [](const boost::system::error_code &ec, std::size_t bytesSent) {
        if (ec) {
          std::cerr << "Send error: " << ec.message() << "\n";
        }
        // No else-case: success means bytesSent == bytes
      });
}
