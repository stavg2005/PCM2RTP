#pragma once
#include <boost/asio/io_context.hpp>
#include <string>
#include "PCMReceiver.hpp"

class SessionManager : public std::enable_shared_from_this<SessionManager>{
public:
    SessionManager(boost::asio::io_context& io,
                   uint16_t localPort,
                   const std::string& remoteAddr,
                   uint16_t remotePort);
          SessionManager(boost::asio::io_context& io,
                   boost::asio::ip::udp::socket&& socket,
                   const std::string& remoteAddr,
                   uint16_t remotePort);
    void start(std::filesystem::path path, std::string file_name);
    void stop();

private:
    PCMReceiver receiver_;
};
    