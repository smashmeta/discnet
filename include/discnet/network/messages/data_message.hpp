/*
 *
 */

#pragma once

#include <vector>
#include <discnet/typedefs.hpp>
#include <discnet/network/buffer.hpp>
#include <discnet/network/messages/header.hpp>

namespace discnet::network::messages
{
    struct data_message_t
    {
        uint16_t m_identifier = 0;
        std::vector<discnet::byte_t> m_buffer = {};
    };

    using expected_data_message_t = std::expected<data_message_t, std::string>;

    struct data_message_codec_t
    {
        static const size_t s_data_message_header_size = 4;
        static const message_type_e s_message_type = message_type_e::data_message;

        DISCNET_EXPORT static size_t encoded_size(const data_message_t& message);
        DISCNET_EXPORT static bool encode(network::buffer_t& buffer, const data_message_t& message);
        DISCNET_EXPORT static expected_data_message_t decode(network::buffer_t& buffer);
    };

    DISCNET_EXPORT bool operator==(const data_message_t& lhs, const data_message_t& rhs);
} // !namespace discnet::network::messages