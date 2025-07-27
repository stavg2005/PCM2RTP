cmake_minimum_required(VERSION 3.15)
project(RTP-BUILDER)

set(CMAKE_CXX_STANDARD 17)
include(FetchContent)

# Optionally include only the needed Boost libs
set(BOOST_ENABLE_CMAKE ON)
set(BOOST_INCLUDE_LIBRARIES beast asio system core foreach)

FetchContent_Declare(
  Boost
  URL https://github.com/boostorg/boost/releases/download/boost-1.88.0/boost-1.88.0-cmake.zip
  DOWNLOAD_EXTRACT_TIMESTAMP ON
)

FetchContent_MakeAvailable(Boost)

# ---------- Server ----------
add_executable(server
    Src/packet.cpp
    Src/alaw.cpp
    Src/PCMReciver.cpp
    Src/Test_PCMReciver.cpp
)
target_link_libraries(server PRIVATE  Boost::foreach Boost::beast Boost::asio Boost::system Boost::core )



