#include "../include/RTPPacketizer.hpp"
#include "packet.hpp"
#include <cstdint>
#include <ctime>

RTPPacketizer::RTPPacketizer(uint8_t payloadType,
                             uint32_t ssrc,
                             uint32_t timestampIncrement)
  : payloadType_(payloadType)
  , ssrc_(ssrc)
  , timestampIncrement_(timestampIncrement)
{}

//gets an aleady encoded payload ,constructs an RTP Packet ,  Serialize the packet into the caller's buffer and returns how many bytes wrote for future async_send_to function
size_t RTPPacketizer::packetize(const std::vector<uint8_t>& payload,bool marker,boost::span<uint8_t> outBuffer){
    RTPPacket::Header hdr{
        /* padding */    false,            // Padding bit: when true, packet has extra padding bytes at the end (for alignment or trailers). We don't use padding, so it's false.
        /* version */    version_,         // RTP version (always 2)
        /* payload_type */ payloadType_,   // Payload type (e.g. 8 for PCMA)
        /* marker */     marker,           // Marker bit: payload‑specific flag (e.g. start of talkspurt or end of frame). We pass it in as needed.
        /* sequence_num */ sequenceNum_++,  // Packet sequence number, increments per packet
        /* timestamp */  timestamp_,        // Timestamp for this packet's first sample
        /* ssrc */       ssrc_,             // Synchronization source identifier
        /* csrc_list */  {},                // Contributing sources (unused)
        /* extension */  std::nullopt       // Header extension (unused)
    };
  RTPPacket packet{std::move(hdr),payload};

  auto maybeSpan = packet.to_buffer(outBuffer);
  if(!maybeSpan) // catch error
    return 0;
    
  timestamp_ += timestampIncrement_;
  return maybeSpan->size();
}