#include "SessionManager.hpp"
#include <boost/asio/io_context.hpp>
#include <iostream>
#include <csignal>
#include <memory>

// Global pointer to SessionManager so we can stop it on Ctrl+C
std::shared_ptr<SessionManager> g_session;

void signalHandler(int signal) {
    std::cerr << "\nCaught SIGINT (Ctrl+C). Shutting down...\n";
    //וfuture exit logic...
    std::exit(0);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0]
                  << " <localPort> <remoteAddr> <remotePort>\n";
        return 1;
    }

    uint16_t localPort = static_cast<uint16_t>(std::stoi(argv[1]));
    std::string remoteAddr = argv[2];
    uint16_t remotePort = static_cast<uint16_t>(std::stoi(argv[3]));

    // Set up Ctrl+C handler
    std::signal(SIGINT, signalHandler);

    try {
        boost::asio::io_context io;

        // Create the session manager
        g_session = std::make_shared<SessionManager>(io, localPort, remoteAddr, remotePort);

        // Start receiving, packetizing, and sending
        g_session->start();

        std::cout << " Session started. Listening on UDP port " << localPort
                  << " and sending RTP to " << remoteAddr << ":" << remotePort << "\n";

        // Run the IO loop (this never returns until you stop the program)
        io.run();

    } catch (const std::exception& ex) {
        std::cerr << " Fatal error: " << ex.what() << "\n";
        return 2;
    }

    return 0;
}
