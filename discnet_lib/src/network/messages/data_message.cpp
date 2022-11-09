/*
 *
 */

#include <discnet_lib/network/messages/data_message.hpp>

namespace discnet::network::messages
{
    size_t data_message_codec_t::encoded_size(const data_message_t& message)
    {
        const size_t s_header_size = header_codec_t::size();
        const size_t message_size = s_data_message_header_size + message.m_buffer.size();
        return s_header_size + message_size;
    }

    bool data_message_codec_t::encode(network::buffer_t& buffer, const data_message_t& message)
    {
        const size_t message_size = encoded_size(message);
        if (buffer.remaining_bytes() < message_size)
        {
            return false;
        }

        header_codec_t::encode(buffer, message_size, s_message_type);

        buffer.append(native_to_network((uint16_t)message.m_identifier));
        buffer.append(native_to_network((uint16_t)message.m_buffer.size()));
        buffer.append(std::span(message.m_buffer.data(), message.m_buffer.size()));

        return true;
    }

    expected_data_message_t data_message_codec_t::decode(network::buffer_t& buffer)
    {
        if (buffer.bytes_left_to_read() < s_data_message_header_size)
        {
            return std::unexpected("not enough bytes in buffer to read data_message.");
        }
        
        uint16_t identifier = network_to_native(buffer.read_uint16());
        uint16_t size = network_to_native(buffer.read_uint16());
        buffer_span_t data = buffer.read_buffer(size);
        
        data_message_t result{.m_identifier = identifier};
        result.m_buffer.assign(data.begin(), data.end());

        return result;
    }

    bool operator==(const data_message_t& lhs, const data_message_t& rhs)
    {
        return lhs.m_identifier == rhs.m_identifier &&
            std::equal(lhs.m_buffer.begin(), lhs.m_buffer.end(), rhs.m_buffer.begin(), rhs.m_buffer.end());
    }
} // ! namespace discnet::network::messages