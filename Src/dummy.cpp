#include <boost/asio.hpp>
#include <iostream>
#include <vector>

using boost::asio::ip::udp;

int main() {
  boost::asio::io_context io_context;

  // Listen on UDP port 6000
  udp::socket socket(io_context, udp::endpoint(udp::v4(), 6000));

  std::cout << "UDP Receiver listening on port 6000...\n";

  std::vector<char> recv_buffer(2048);
  udp::endpoint remote_endpoint;

  std::function<void()> do_receive; // <-- Use std::function
    int count =1;
  do_receive = [&]() {
    socket.async_receive_from(
        boost::asio::buffer(recv_buffer), remote_endpoint,
        [&](boost::system::error_code ec, std::size_t bytes_recvd) {
          if (!ec && bytes_recvd > 0) {
            std::cout << "frame: " <<count++<<"\n";
            std::cout << "Received " << bytes_recvd << " bytes from "
                      << remote_endpoint.address().to_string() << ":"
                      << remote_endpoint.port() << "\n";
          } else {
            std::cerr << "Receive error: " << ec.message() << "\n";
          }

          // Recursive call works fine now
          do_receive();
        });
  };

  // Start the first receive
  do_receive();

  // Run IO loop
  io_context.run();

  return 0;
}
