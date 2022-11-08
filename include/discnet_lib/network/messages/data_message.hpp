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

    using expected_data_message_t = std::expected<data_message_t, std::string>;

    struct data_message_codec_t
    {
        static const size_t s_data_message_header_size = 4;
        static const message_type_e s_message_type = message_type_e::data_message;

        static size_t encoded_size(const data_message_t& message)
        {
            const size_t s_header_size = header_codec_t::size();
            const size_t message_size = s_data_message_header_size + message.m_buffer.size();
            return s_header_size + message_size;
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

        static expected_data_message_t decode(network::buffer_t& buffer)
        {
            if (buffer.bytes_left_to_read() < s_data_message_header_size)
            {
                return std::unexpected("not enough bytes in buffer to read data_message.");
            }
            
            uint16_t identifier = boost::endian::big_to_native(buffer.read_uint16());
            uint16_t size = boost::endian::big_to_native(buffer.read_uint16());
            buffer_span_t data = buffer.read_buffer(size);
            
            data_message_t result{.m_identifier = identifier};
            result.m_buffer.assign(data.begin(), data.end());

            return result;
        }
    };
} // !namespace discnet::network::messages