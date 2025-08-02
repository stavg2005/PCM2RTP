#pragma  once
#include <basetsd.h>
#include <vector>
#include <cstdint>
#include "boost/core/span.hpp"

class RTPPacketizer{
    public:

    //gets an aleady encoded payload ,constructs an RTP Packet ,  Serialize the packet into the caller’s buffer and returns how many bytes wrote for future async_send_to function
    RTPPacketizer(uint8_t payload_type,uint32_t ssrc,uint32_t timestampIncrement);

    size_t packetize(const std::vector<uint8_t>& payload,bool marker,boost::span<uint8_t> outBuffer);

    private:
        uint8_t payloadType_;
        uint32_t ssrc_;
        uint16_t sequenceNum_ =0;
        uint32_t timestamp_ =0;
        uint32_t timestampIncrement_;
        static constexpr uint8_t version_ =2;
};