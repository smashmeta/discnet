/*
 *
 */

#pragma once

#include <variant>
#include <expected>
#include <boost/endian.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <discnet/discnet.hpp>
#include <discnet/typedefs.hpp>
#include <discnet/network/buffer.hpp>
#include <discnet/network/messages/discovery_message.hpp>
#include <discnet/network/messages/data_message.hpp>

namespace discnet::network::messages
{
    using message_variant_t = std::variant<discovery_message_t, data_message_t>;
    using message_list_t = std::vector<message_variant_t>;

    struct packet_t
    {
        message_list_t m_messages = {};
    };

    using expected_packet_t = std::expected<packet_t, std::string>;

    struct packet_codec_t
    {
        static const size_t s_packet_size_field_size = 4;
        static const size_t s_message_count_field_size = 2;
        static const size_t s_checksum_size = 4;
        static const size_t s_header_size = s_packet_size_field_size + s_message_count_field_size + s_checksum_size;
        
        DISCNET_EXPORT static std::expected<bool,std::string> validate_packet(network::buffer_t& buffer);
        DISCNET_EXPORT static bool encode(discnet::network::buffer_t& buffer, const message_list_t& messages);
        DISCNET_EXPORT static expected_packet_t decode(network::buffer_t& buffer);
    };
} // !namespace discnet::network::messages