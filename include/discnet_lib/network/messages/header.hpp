/*
 *
 */

#pragma once

#include <expected>
#include <boost/endian.hpp>
#include <discnet_lib/discnet_lib.hpp>
#include <discnet_lib/typedefs.hpp>
#include <discnet_lib/network/buffer.hpp>

namespace discnet::network::messages
{
    enum class message_type_e : uint16_t
    {
        discovery_message = 1,
        data_message = 2,
        command_message = 3
    };

    /*
    header [
        size: 24,
        type: discovery_message (1)
    ]
    */
    struct header_t
    {
        uint32_t m_size = init_required;
        message_type_e m_type = init_required;
    };

    using expected_header_t = std::expected<header_t, std::string>;

    struct header_codec_t
    {
        const static size_t s_header_size = 6;

        static size_t size()
        {
            return s_header_size;
        }

        static bool encode(network::buffer_t& buffer, size_t size, message_type_e message_type)
        {
            if (buffer.remaining_bytes() < s_header_size)
            {
                return false;
            }

            buffer.append(boost::endian::native_to_big((uint32_t)size));
            buffer.append(boost::endian::native_to_big((uint16_t)message_type));

            return true;
        }

        static expected_header_t decode(network::buffer_t& buffer)
        {
            if (buffer.bytes_left_to_read() < s_header_size)
            {
                return std::unexpected("not enough bytes in buffer to read header.");
            }

            uint32_t size = boost::endian::big_to_native(buffer.read<uint32_t>());
            message_type_e type = (message_type_e)boost::endian::big_to_native(buffer.read<uint16_t>());
            return header_t{.m_size = size, .m_type = type};
        }
    };
} // namespace discnet::network::messages