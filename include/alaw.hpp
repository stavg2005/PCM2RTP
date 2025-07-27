#pragma once
#include <boost/asio.hpp>
#include <cstdint>    // for uint8_t, uint16_t, uint32_t, etc.
#include <vector>     // for std::vector
std::vector<int16_t> decode_alaw(const std::vector<uint8_t>& data);

std::vector<uint8_t> encode_alaw(const std::vector<int16_t>& data);
