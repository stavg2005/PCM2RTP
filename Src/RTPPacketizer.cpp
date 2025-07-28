#include "../include/RTPPacketizer.hpp"
#include "packet.hpp"
#include <cstdint>
#include <ctime>
#include <winnt.h>

RTPPacketizer::RTPPacketizer(uint8_t payloadType,
                             uint32_t ssrc,
                             uint32_t timestampIncrement)
  : payloadType_(payloadType)
  , ssrc_(ssrc)
  , timestampIncrement_(timestampIncrement)
{}

//gets an aleady encoded payload ,constructs an RTP Packet ,  Serialize the packet into the caller’s buffer and returns how many bytes wrote for future async_send_to function
size_t RTPPacketizer::packetize(const std::vector<uint8_t>& payload,bool marker,boost::span<uint8_t> outBuffer){
     RTPPacket::Header hdr{
    /*padding*/    false,
    /*version*/    version_,
    /*payload_type*/payloadType_,
    /*marker*/     marker,
    /*sequence_num*/sequenceNum_++,
    /*timestamp*/  timestamp_,
    /*ssrc*/       ssrc_,
    /*csrc_list*/  {},
    /*extension*/  std::nullopt
  };
  RTPPacket packet{std::move(hdr),payload};

  auto maybeSpan = packet.to_buffer(outBuffer);
  if(!maybeSpan) // catch error
    return 0;
    
  timestamp_ += timestampIncrement_;
  return maybeSpan->size();
}