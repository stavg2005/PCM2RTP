#pragma once
#include "boost/core/span.hpp"
#include <boost/asio.hpp>
#include <cstdint> // for uint8_t, uint16_t, uint32_t, etc.
#include <optional>
#include <vector> // for std::vector
[[nodiscard]] uint32_t generate_ssrc();

struct RTPPacket {
  struct Header {
    // Used to indicate if there are extra padding bytes at the end of the RTP
    // packet. Padding may be used to fill up a block of certain size, for
    // example as required by an encryption algorithm. The last byte of the
    // padding contains the number of padding bytes that were added (including
    // itself).
    bool padding;

    // Indicates the version of the protocol. Current version is 2.
    uint8_t version;

    // Indicates the format of the payload and thus determines its
    // interpretation by the application. Values are profile specific and may be
    // dynamically assigned.
    uint8_t payload_type;

    // Signaling used at the application level in a profile-specific manner.
    // If it is set, it means that the current data has some special relevance
    // for the application.
    bool marker;

    // The sequence number is incremented for each RTP data packet sent and is
    // to be used by the receiver to detect packet loss and to accommodate
    // out-of-order delivery. The initial value of the sequence number should be
    // randomized to make known-plaintext attacks on Secure Real-time Transport
    // Protocol more difficult.
    uint16_t sequence_num = 0;

    // Used by the receiver to play back the received samples at appropriate
    // time and interval. When several media streams are present, the timestamps
    // may be independent in each stream. The granularity of the timing is
    // application specific. For example, an audio application that samples data
    // once every 125 μs (8 kHz, a common sample rate in digital telephony)
    // would use that value as its clock resolution. Video streams typically use
    // a 90 kHz clock. The clock granularity is one of the details that is
    // specified in the RTP profile for an application.
    uint32_t timestamp = 0;

    // Synchronization source identifier uniquely identifies the source of a
    // stream. The synchronization sources within the same RTP session will be
    // unique.
    uint32_t ssrc = 0;

    // (32 bits each, the number of entries is indicated by the CSRC count
    // field) Contributing source IDs within a stream which has been generated
    // from multiple sources.
    std::vector<uint32_t> csrc_list;

    // Optional, presence indicated by Extension field.
    // Contains application specific data.
    struct Extension {
      // Profile-specific identifier.
      uint16_t id = 0;

      // Extension data.
      std::vector<uint32_t> data;

      Extension() = default;
      Extension(uint16_t id, std::vector<uint32_t> data);
      Extension(Extension &&other) noexcept;
      Extension(const Extension &other) = default;

      ~Extension() = default;

      Extension &operator=(Extension &&other) noexcept;
      Extension &operator=(const Extension &other) = default;
    };

    std::optional<Extension> extension;

    Header() = default;
    Header(bool padding, uint8_t version, uint8_t payload_type, bool marker,
           uint16_t sequence_num, uint32_t timestamp, uint32_t ssrc,
           std::vector<uint32_t> csrc_list, std::optional<Extension> extension);
    Header(Header &&other) noexcept;
    Header(const Header &other) = default;

    ~Header() = default;

    Header &operator=(Header &&other) noexcept;
    Header &operator=(const Header &other) = default;
  } header;

  // RTP data (audio/video):
  std::vector<uint8_t> data;

  RTPPacket() = default;
  RTPPacket(Header header, std::vector<uint8_t> data);
  RTPPacket(RTPPacket &&other) noexcept;
  RTPPacket(const RTPPacket &other) = default;

  ~RTPPacket() = default;

  RTPPacket &operator=(RTPPacket &&other) noexcept;
  RTPPacket &operator=(const RTPPacket &other) = default;

  [[nodiscard]] static std::optional<RTPPacket>
  from_buffer(const boost::span<uint8_t> &buffer);

  void add_ssrc(uint32_t new_ssrc);
  std::optional<boost::span<uint8_t>>
  to_buffer(const boost::span<uint8_t> &packet_buffer) const;
};
