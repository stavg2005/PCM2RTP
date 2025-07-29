#include "../include/PCMReciver.hpp"
#include <iostream>

int main() {
  boost::asio::io_context io;
  PCMReceiver recv(io, 5005);

  int count = 1;

  // Register the handler ONCE
  recv.GetNextFrameasync([&](const boost::system::error_code &ec,
                             boost::span<const uint8_t> frame) {
    if (ec) {
      std::cerr << ec.message() << "\n";
      return;
    }

    std::cout << "Frame " << count++ << ": " << frame.size() << " bytes\n";
    recv.consumeFrame();
  });

  io.run();
  return 0;
}
