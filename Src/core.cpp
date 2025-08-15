#include <rtpbuilder/core.hpp>
#include <filesystem>
#include <rtpbuilder/SessionManager.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif
int TransmitFile(int argc, char* argv[]){

    if (argc != 6) {
        std::cerr << "Usage: " << argv[0]
                  << " <localPort> <remoteAddr> <remotePort> <file path> <file name>\n";
        return 1;
    }

    uint16_t localPort   = static_cast<uint16_t>(std::stoi(argv[1]));
    std::string remoteIp = argv[2];
    uint16_t remotePort  = static_cast<uint16_t>(std::stoi(argv[3]));
    std::filesystem::path path = static_cast<std::filesystem::path>(argv[4]);
    std::string FileName = argv[5];
    try {
        boost::asio::io_context io;

        
        SessionManager session(io, localPort, remoteIp, remotePort);
        session.start(path,FileName);
        
        // set up signal handling inside Asio
        boost::asio::signal_set signals(io, SIGINT, SIGTERM);
        signals.async_wait([&](const boost::system::error_code&, int sig) {
            std::cerr << "\nCaught signal " <<  " — shutting down gracefully\n";
            session.stop();  
            io.stop();    
        });

        std::cout << " Session started on UDP " << localPort
                  << " sending RTP to " << remoteIp << ":" << remotePort << "\n";

        io.run();  

    } catch (const std::exception& ex) {
        std::cerr << " Fatal error: " << ex.what() << "\n";
        return 2;
    }

    return 0;
}