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

        DISCNET_EXPORT static size_t size();
        DISCNET_EXPORT static bool encode(network::buffer_t& buffer, size_t size, message_type_e message_type);
        DISCNET_EXPORT static expected_header_t decode(network::buffer_t& buffer);
    };
} // namespace discnet::network::messages