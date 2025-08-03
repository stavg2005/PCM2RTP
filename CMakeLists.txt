cmake_minimum_required(VERSION 3.27)
project(rtpbuilder LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------- Fetch Boost (header-only bits + libs you need) ----------
include(FetchContent)

set(BOOST_ENABLE_CMAKE ON)
set(BOOST_INCLUDE_LIBRARIES beast asio system core foreach)

FetchContent_Declare(
  boost
  URL https://github.com/boostorg/boost/releases/download/boost-1.88.0/boost-1.88.0-cmake.zip
  DOWNLOAD_EXTRACT_TIMESTAMP ON
)
FetchContent_MakeAvailable(boost)

# ---------- Build your library ----------
add_library(rtpbuilder STATIC
    src/alaw.cpp
    src/packet.cpp
    src/PCMReceiver.cpp
    src/RTPPacketizer.cpp
    src/RTPTransmitter.cpp
    src/SessionManager.cpp
    src/PacketUtils.cpp
)

# Expose *public* headers
target_include_directories(rtpbuilder
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link Boost bits
target_link_libraries(rtpbuilder
    PUBLIC
        Boost::asio
        Boost::system
        Boost::beast
        Boost::core
        Boost::foreach
)

target_compile_features(rtpbuilder PUBLIC cxx_std_23)

# ---------- Example executable -------------
add_executable(rtp_demo src/main.cpp)
target_link_libraries(rtp_demo PRIVATE rtpbuilder)

add_executable(pcm_sender src/pcm_sender.cpp)
target_link_libraries(pcm_sender PRIVATE rtpbuilder)
# Dummy receiver exe
add_executable(receiver src/dummy.cpp)
target_link_libraries(receiver PRIVATE rtpbuilder)


