#pragma once
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <filesystem>
#include <iostream>
#include <rtpbuilder/SessionManager.hpp>


std::unique_ptr<SessionManager>
TransmitWavFile(boost::asio::io_context &io, uint16_t localPort,
                const std::string &remoteIp, uint16_t remotePort,
                const std::filesystem::path &filePath,
                const std::string &fileName);