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
    // if we ever need a clean shutdown, we can extend PCMReceiver with a stop() method
    std::cout << "SessionManager stopping session (future cleanup here)\n";
}
