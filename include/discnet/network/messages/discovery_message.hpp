/*
 *
 */

#pragma once

#include <vector>
#include <discnet/network/buffer.hpp>
#include <discnet/network/messages/header.hpp>

namespace discnet::network::messages
{
    using jumps_t = std::vector<uint16_t>;

    struct node_t
    {
        uint16_t m_identifier = init_required;
        address_v4_t m_address = init_required;
        jumps_t m_jumps = {};
    };
    
    using nodes_vector_t = std::vector<node_t>;

    /*
    msg [
       id: 1010,
       children: 
       [ 
           entry {id: 1011, ip: 192.200.1.11},
           entry {id: 1012, ip: 192.200.1.12}
       ]
    ]
    */
    struct discovery_message_t
    {
        uint16_t m_identifier = init_required;
        nodes_vector_t m_nodes = {};
    };

    using expected_discovery_message_t = std::expected<discovery_message_t, std::string>;

    struct discovery_message_codec_t
    {
        static const size_t s_node_id_size = sizeof(uint16_t);
        static const size_t s_nodes_array_size = sizeof(uint16_t);
        static const size_t s_nodes_array_element_size = sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t);
        static const message_type_e s_message_type = message_type_e::discovery_message;

        DISCNET_EXPORT static size_t encoded_size(const discovery_message_t& message);
        DISCNET_EXPORT static bool encode(network::buffer_t& buffer, const discovery_message_t& message);
        DISCNET_EXPORT static expected_discovery_message_t decode(network::buffer_t& buffer);
    };

    DISCNET_EXPORT bool operator==(const node_t& lhs, const node_t& rhs);
    DISCNET_EXPORT bool operator==(const discovery_message_t& lhs, const discovery_message_t& rhs);
} // namespace discnet::network::messages