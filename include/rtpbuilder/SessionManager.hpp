#pragma once
#include <boost/asio/io_context.hpp>
#include <string>
#include "PCMReceiver.hpp"

class SessionManager {
public:
    SessionManager(boost::asio::io_context& io,
                   uint16_t localPort,
                   const std::string& remoteAddr,
                   uint16_t remotePort);

    void start(std::filesystem::path path, std::string file_name);
    void stop();

private:
    PCMReceiver receiver_;
};
    