#include <rtpbuilder/core.hpp>
#include <filesystem>
#include <rtpbuilder/SessionManager.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>


std::unique_ptr<SessionManager> TransmitWavFile(
    boost::asio::io_context& io, 
    uint16_t localPort,
    const std::string& remoteIp,
    uint16_t remotePort,
    const std::filesystem::path& filePath,
    const std::string& fileName
) {
    try {
        auto session = std::make_unique<SessionManager>(io, localPort, remoteIp, remotePort);
        session->start(filePath, fileName);
        
        std::cout << "Session started on UDP " << localPort
                  << " sending RTP to " << remoteIp << ":" << remotePort << "\n";
        
        return session;  // Return session for caller to manage
        
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << "\n";
        throw;  // Re-throw to let caller handle
    }
}