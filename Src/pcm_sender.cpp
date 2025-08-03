
#include <boost/asio.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

using namespace std::chrono_literals;
namespace asio = boost::asio;
using udp      = asio::ip::udp;

class PcmSender : public std::enable_shared_from_this<PcmSender> {
public:
    PcmSender(asio::io_context& io,
              std::string_view  pcmPath,
              udp::endpoint     dest,
              std::size_t       bytesPerPkt,
              std::chrono::milliseconds period = 20ms)
        : socket_{io, udp::v4()},
          timer_{io},
          file_ {pcmPath.data(), std::ios::binary},
          dest_ {std::move(dest)},
          chunk_(bytesPerPkt),
          period_{period}
    {
        if (!file_)
            throw std::runtime_error("✘ can't open PCM file");

        std::cout << "✓ streaming " << pcmPath << " → " << dest_.address()
                  << ':' << dest_.port() << "   every " << period_.count() << " ms ("
                  << chunk_.size() << " bytes)\n";
    }

    void start()
    {
        nextTick_ = asio::steady_timer::clock_type::now(); // wall-clock zero
        tick();                                            // 🚀 kick-off
    }

private:
    /* core “metronome” loop */
    void tick()
    {
        if (!readChunk()) return;       // EOF → stop

        auto self = shared_from_this(); // keep object alive

        socket_.async_send_to(asio::buffer(chunk_), dest_,
            [this, self](boost::system::error_code ec, std::size_t /*bytes*/) {
                if (ec) {
                    std::cerr << "send error: " << ec.message() << '\n';
                    return;
                }

                /* schedule **absolute** next deadline */
                nextTick_ += period_;
                while (nextTick_ <= asio::steady_timer::clock_type::now())
                    nextTick_ += period_;                 // catch-up if we fell behind

                timer_.expires_at(nextTick_);
                timer_.async_wait([this, self](boost::system::error_code ec2) {
                    if (!ec2) tick();                     // loop
                });
            });
    }

    bool readChunk()
    {
        file_.read(reinterpret_cast<char*>(chunk_.data()), chunk_.size());
        std::size_t n = static_cast<std::size_t>(file_.gcount());
        if (n == 0) return false;             // EOF

        if (n < chunk_.size())                // tail (< 1 frame) — shrink vector
            chunk_.resize(n);

        return true;
    }

    /* members */
    udp::socket                        socket_;
    asio::steady_timer                 timer_;
    std::ifstream                      file_;
    udp::endpoint                      dest_;
    std::vector<uint8_t>               chunk_;
    std::chrono::milliseconds          period_;
    asio::steady_timer::time_point     nextTick_;
};

/* ---------- main entry ---------- */
int main(int argc, char* argv[])
{
    if (argc != 5) {
        std::cerr <<
            "usage: " << argv[0] << " <pcm_file> <remote_ip> <remote_port> <bytes_per_packet>\n"
            "ex   : " << argv[0] << " speech.pcm 127.0.0.1 6000 320\n";
        return 1;
    }

    try {
        asio::io_context io;

        udp::endpoint dest(
            asio::ip::make_address(argv[2]),
            static_cast<unsigned short>(std::stoi(argv[3])));

        auto sender = std::make_shared<PcmSender>(
            io, argv[1], dest, static_cast<std::size_t>(std::stoul(argv[4])));

        sender->start();
        io.run();
    }
    catch (const std::exception& ex) {
        std::cerr << "fatal: " << ex.what() << '\n';
        return 1;
    }
}
