// SessionManager.cpp
#include "rtpbuilder/PCMReceiver.hpp"
#include <./RtpBuilder/SessionManager.hpp>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <thread>


SessionManager::SessionManager(boost::asio::io_context &io, uint16_t localPort,
                               const std::string &remoteAddr,
                               uint16_t remotePort)
    : receiver_(io, localPort, remoteAddr, remotePort), io_(io) {}

SessionManager::SessionManager(boost::asio::io_context &io,
                               boost::asio::ip::udp::socket &&socket,
                               const std::string &remoteAddr,
                               uint16_t remotePort)
    : receiver_(io, std::move(socket), remoteAddr, remotePort), io_(io) {}

void SessionManager::start_managed_session(
    std::filesystem::path path, std::string file_name, uint64_t session_id,
    std::function<void(uint64_t)> cleanup_callback) {
  std::cout << "SessionManager starting managed session " << session_id << "\n";

  session_id_ = session_id;
  cleanup_callback_ = std::move(cleanup_callback);

  try {
    start(path, file_name);
  } catch (const std::exception &e) {
    std::cout << "Error starting managed session " << session_id << ": "
              << e.what() << "\n";
    cleanup();
    return;
  }

  setup_completion_timer(path);
}

void SessionManager::start(std::filesystem::path path, std::string file_name) {
  std::cout << "SessionManager started PCMReceiver\n";
  receiver_.read_pcm_from_wav(path, file_name);
}

void SessionManager::stop() {
  std::cout << "SessionManager stopping...\n";

  // Cancel timer but don't destroy it
  if (completion_timer_) {
    try {
      completion_timer_->cancel();
      std::cout << "Timer cancelled\n";
    } catch (const std::exception &e) {
      std::cout << "Timer cancel error: " << e.what() << "\n";
    }
  }

  receiver_.stop();
  std::cout << "SessionManager stop complete\n";
}

uint16_t SessionManager::get_local_port() const {
  return receiver_.get_local_port();
}

void SessionManager::setup_completion_timer(const std::filesystem::path &path) {
try {
        auto duration_ms = calculate_transmission_duration(path) * 120 / 100;

        std::cout << "Session " << *session_id_ << " expected to complete in "
                  << duration_ms << "ms\n";

        completion_timer_ = std::make_unique<boost::asio::steady_timer>(io_);
        completion_timer_->expires_after(std::chrono::milliseconds(duration_ms));

        // Use boost::asio::post() to defer cleanup
        completion_timer_->async_wait(
            [weak_self = std::weak_ptr<SessionManager>(shared_from_this())](
                const boost::system::error_code &ec) {
              if (ec) {
                if (ec != boost::asio::error::operation_aborted) {
                  std::cout << "Timer error: " << ec.message() << "\n";
                }
                return;
              }

              auto self = weak_self.lock();
              if (!self) return;

              std::cout << "Session " << *self->session_id_
                        << " completed (timer-based)\n";
              
              if (self->session_id_ && self->cleanup_callback_) {
                  auto session_id_copy = *self->session_id_;
                  auto callback = std::move(self->cleanup_callback_);
                  
                  // Clear state
                  self->session_id_.reset();
                  self->cleanup_callback_ = nullptr;
                  
                  // Stop operations
                  self->stop();
                  
                  // CORRECT: Use boost::asio::post() with io_context reference
                  boost::asio::post(self->io_, [callback = std::move(callback), session_id_copy]() {
                      std::cout << "Executing deferred cleanup\n";
                      callback(session_id_copy);
                  });
              }
            });

    } catch (const std::exception &e) {
        std::cout << "Error setting up completion timer: " << e.what() << "\n";
        cleanup();
    }
}

void SessionManager::cleanup() {
  try {
    std::cout << "Cleaning up managed session " << *session_id_ << "\n";

    // Stop operations
    stop();

    // DON'T TOUCH THE TIMER!
    // completion_timer_.reset(); ← REMOVE THIS!

    // Just do the callback
    if (session_id_ && cleanup_callback_) {
      auto session_id_copy = *session_id_;
      auto callback = std::move(cleanup_callback_);

      session_id_.reset();
      cleanup_callback_ = nullptr;

      // Call callback directly - no thread needed!
      callback(session_id_copy);
    }

    std::cout << "Cleanup complete\n";

  } catch (const std::exception &e) {
    std::cerr << "Exception during cleanup: " << e.what() << "\n";
  }
}

uint64_t SessionManager::calculate_transmission_duration(
    const std::filesystem::path &wav_path) {
  try {
    if (!std::filesystem::exists(wav_path)) {
      std::cerr << "File does not exist: " << wav_path << "\n";
      throw std::runtime_error("WAV file does not exist");
    }

    auto file_size = std::filesystem::file_size(wav_path);
    std::cout << "File size for \"" << wav_path << "\": " << file_size
              << " bytes\n";

    if (file_size <= 44) {
      std::cerr << "Invalid WAV file size for " << wav_path << ": " << file_size
                << " bytes\n";
      throw std::runtime_error("WAV file too small or invalid");
    }

    const size_t WAV_HEADER_SIZE = 44;
    auto data_size = file_size - WAV_HEADER_SIZE;
    const int MS_PER_PACKET = 20;
    const int FRAME_SIZE_BYTES = 320;

    auto num_packets = (data_size + FRAME_SIZE_BYTES - 1) / FRAME_SIZE_BYTES;
    auto duration_ms = num_packets * MS_PER_PACKET;

    std::cout << "Calculated duration for \"" << wav_path
              << "\": " << duration_ms << "ms (" << num_packets
              << " packets)\n";

    return std::max<uint64_t>(1000, duration_ms); // Minimum 1 second

  } catch (const std::exception &e) {
    std::cerr << "Error calculating duration for " << wav_path << ": "
              << e.what() << "\n";
    throw;
  }
}