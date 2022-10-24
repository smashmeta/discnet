/*
 *
 */

#pragma once

#include <vector>
#include <discnet_lib/typedefs.hpp>
#include <discnet_lib/network/buffer.hpp>
#include <discnet_lib/network/messages/header.hpp>

namespace discnet::network::messages
{
    struct data_message_t
    {
        uint16_t m_identifier = init_required;
        std::vector<discnet::byte_t> m_buffer = {};
    };

    struct data_message_codec_t
    {
        static const size_t s_data_message_header_size = 4;
        static const message_type_e s_message_type = message_type_e::data_message;

        static size_t encoded_size(const data_message_t& message)
        {
            const size_t header_size = header_codec_t::size();
            const size_t message_size = s_data_message_header_size + message.m_buffer.size();
            return header_size + message_size;
        }

        static bool encode(network::buffer_t& buffer, const data_message_t& message)
        {
            const size_t message_size = encoded_size(message);
            if (buffer.remaining_bytes() < message_size)
            {
                return false;
            }

            header_codec_t::encode(buffer, message_size, s_message_type);

            buffer.append(boost::endian::native_to_big((uint16_t)message.m_identifier));
            buffer.append(boost::endian::native_to_big((uint16_t)message.m_buffer.size()));
            buffer.append(std::span(message.m_buffer.data(), message.m_buffer.size()));

            return true;
        }
    };
} // !namespace discnet::network::messages