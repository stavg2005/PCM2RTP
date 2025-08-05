
#include <rtpbuilder/SessionManager.hpp>
#include <iostream>

SessionManager::SessionManager(boost::asio::io_context& io,
                               uint16_t localPort,
                               const std::string& remoteAddr,
                               uint16_t remotePort)
    : receiver_(io, localPort, remoteAddr, remotePort)
{
}

void SessionManager::start() {
    receiver_.start();
    std::cout << "SessionManager started PCMReceiver\n";
}

void SessionManager::stop() {
    std::cout << "SessionManager initiating shutdown...\n";
    
        receiver_.stop();
    
    std::cout << "SessionManager shutdown complete.\n";
}
