/*
 *
 */

#include <discnet_lib/network/messages/header.hpp>

namespace discnet::network::messages
{
    size_t header_codec_t::size()
    {
        return s_header_size;
    }

    bool header_codec_t::encode(network::buffer_t& buffer, size_t size, message_type_e message_type)
    {
        if (buffer.remaining_bytes() < s_header_size)
        {
            return false;
        }

        buffer.append(native_to_network((uint32_t)size));
        buffer.append(native_to_network((uint16_t)message_type));

        return true;
    }

    expected_header_t header_codec_t::decode(network::buffer_t& buffer)
    {
        if (buffer.bytes_left_to_read() < s_header_size)
        {
            return std::unexpected("not enough bytes in buffer to read header.");
        }

        uint32_t size = network_to_native(buffer.read_uint32());
        message_type_e type = (message_type_e)network_to_native(buffer.read_uint16());
        return header_t{.m_size = size, .m_type = type};
    }
} // ! namespace discnet::network::messages