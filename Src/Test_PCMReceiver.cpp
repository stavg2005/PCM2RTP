#include "../include/PCMReciver.hpp"
#include <iostream>


int main() {
  boost::asio::io_context io;
  PCMReceiver recv(io, 5005);

  int count = 1;
  std::function<void()> requestNext;
  requestNext = [&]() {
    recv.GetNextFrameasync(
        [&](const boost::system::error_code &ec, std::vector<uint8_t> frame) {
          if (ec) {
            std::cerr << "Error: " << ec.message() << "\n";
            return;
          }
          std::cout << "Async frame " << count++ << ": " << frame.size()
                    << " bytes\n";
          
            requestNext();
        });
  };

  requestNext();
  io.run();
  return 0;
}
