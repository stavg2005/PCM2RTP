// SessionManager.hpp
#pragma once
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include "PCMReceiver.hpp"

class SessionManager : public std::enable_shared_from_this<SessionManager> {
public:
    SessionManager(boost::asio::io_context& io,
                   uint16_t localPort,
                   const std::string& remoteAddr,
                   uint16_t remotePort);
                   
    SessionManager(boost::asio::io_context& io,
                   boost::asio::ip::udp::socket&& socket,
                   const std::string& remoteAddr,
                   uint16_t remotePort);

    void start_managed_session(std::filesystem::path path, 
                              std::string file_name,
                              uint64_t session_id,
                              std::function<void(uint64_t)> cleanup_callback);
    
    void start(std::filesystem::path path, std::string file_name);
    void stop();
    
    uint64_t get_session_id() const { return session_id_.value_or(0); }
    uint16_t get_local_port() const;
    bool is_managed() const { return session_id_.has_value(); }

private:
    void setup_completion_timer(const std::filesystem::path& path);
    void cleanup();
    uint64_t calculate_transmission_duration(const std::filesystem::path& path);
    
    PCMReceiver receiver_;
    boost::asio::io_context& io_;
    
    std::optional<uint64_t> session_id_;
    std::function<void(uint64_t)> cleanup_callback_;
    std::unique_ptr<boost::asio::steady_timer> completion_timer_;
};