#include <boost/asio.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

using namespace std::chrono_literals;
namespace asio = boost::asio;
using udp = asio::ip::udp;

int main(int argc, char *argv[]) {
  if (argc != 5) {
    std::cerr << "usage: " << argv[0]
              << " <pcm_file> <remote_ip> <remote_port> <bytes_per_packet>\n";
    return 1;
  }

  try {
    asio::io_context io;
    udp::socket socket(io, udp::v4());
    udp::endpoint dest(asio::ip::make_address(argv[2]),
                       static_cast<unsigned short>(std::stoi(argv[3])));

    std::ifstream file(argv[1], std::ios::binary);
    if (!file) {
      std::cerr << "can't open PCM file\n";
      return 1;
    }

    std::size_t chunk_size = static_cast<std::size_t>(std::stoul(argv[4]));
    std::vector<uint8_t> chunk(chunk_size);

    std::cout << "streaming " << argv[1] << " to " << argv[2] << ":" << argv[3]
              << " every 20ms (" << chunk_size << " bytes)\n";

    auto start_time = std::chrono::steady_clock::now();
    int packet_count = 0;

    while (true) {
      file.read(reinterpret_cast<char *>(chunk.data()), chunk_size);
      std::size_t bytes_read = static_cast<std::size_t>(file.gcount());

      // encode + rtp

      if (bytes_read == 0)
        break; // EOF

      if (bytes_read < chunk_size) {
        chunk.resize(bytes_read);
      }

      socket.send_to(asio::buffer(chunk), dest);

      // Calculate absolute next send time
      packet_count++;
      auto next_time =
          start_time + std::chrono::milliseconds(packet_count * 20);
      std::this_thread::sleep_until(next_time);
    }

  } catch (const std::exception &ex) {
    std::cerr << "error: " << ex.what() << '\n';
    return 1;
  }
}