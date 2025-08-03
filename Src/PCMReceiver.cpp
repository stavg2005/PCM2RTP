#include <iostream>
#include <rtpbuilder/PCMReceiver.hpp>
#include <rtpbuilder/PacketUtils.hpp>
#include <rtpbuilder/packet.hpp>

PCMReceiver::PCMReceiver(net::io_context &io, uint16_t localPort,
                         const std::string &remoteIp, uint16_t remotePort)
    : socket_(io, udp::endpoint(udp::v4(), localPort)),
      packetizer_(/* payloadType */ 8, generate_ssrc(), SAMPLES_PER_FRAME),
      transmitter_(io, remoteIp, remotePort) {
  std::cout << "PCMReceiver created on port " << localPort << std::endl;
}

void PCMReceiver::start() { doReceive(); }

void PCMReceiver::doReceive() {
  socket_.async_receive_from(
      boost::asio::buffer(buffer_), senderEndpoint_,
      [this](boost::system::error_code ec, std::size_t bytesReceived) {
        if (ec) {
          std::cerr << "Recv error: " << ec.message() << '\n';
          return;
        }

        // convert PCM → RTP into rtpBuf_
        std::size_t pktLen = PacketUtils::packet2rtp(
            {buffer_.data(), bytesReceived},              // source
            packetizer_, {rtpBuf_.data(), rtpBuf_.size()} // destination
        );
        if (pktLen == 0) {
          std::cerr << "packetize failed\n";
          doReceive();
          return;
        }

        
        transmitter_.asyncSend({rtpBuf_.data(), pktLen}, pktLen);

        doReceive(); 
      });
}
