cmake_minimum_required(VERSION 3.27)
project(RTP-BUILDER)
include_directories(${CMAKE_SOURCE_DIR}/include)
set(CMAKE_CXX_STANDARD 23)
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
add_executable(rtp_builder
  Src/PCMReceiver.cpp
  Src/RTPPacketizer.cpp
  Src/RTPTransmitter.cpp
  Src/alaw.cpp
  Src/packet.cpp
  Src/Test_PCMReceiver.cpp
)

# if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
#     set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
# set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
#     add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g -fno-common)
#     add_link_options(-fsanitize=address -fuse-ld=lld )
#         target_compile_options(rtp_builder PRIVATE -stdlib=libc++)
#     target_link_options(rtp_builder PRIVATE -stdlib=libc++)
# endif()

target_link_libraries(rtp_builder PRIVATE  Boost::foreach Boost::beast Boost::asio Boost::system Boost::core )



