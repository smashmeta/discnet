/*
 *
 */

#pragma once

#include <vector>
#include <discnet_lib/network/buffer.hpp>
#include <discnet_lib/network/messages/header.hpp>

namespace discnet::network::messages
{
    using jumps_t = std::vector<uint16_t>;

    struct node_t
    {
        uint16_t m_identifier = init_required;
        address_v4_t m_address = init_required;
        jumps_t m_jumps = {};

        bool operator==(const node_t& node) const
        {
            return m_identifier == node.m_identifier &&
                m_address == node.m_address &&
                std::equal(m_jumps.begin(), m_jumps.end(), node.m_jumps.begin(), node.m_jumps.end());
        }
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

        bool operator==(const discovery_message_t& message) const
        {
            return m_identifier == message.m_identifier &&
                std::equal(m_nodes.begin(), m_nodes.end(), message.m_nodes.begin(), message.m_nodes.end());
        }
    };

    using expected_discovery_message_t = std::expected<discovery_message_t, std::string>;

    struct discovery_message_codec_t
    {
        static const size_t s_node_id_size = sizeof(uint16_t);
        static const size_t s_nodes_array_size = sizeof(uint16_t);
        static const size_t s_nodes_array_element_size = 
            sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t);
        static const message_type_e s_message_type = message_type_e::discovery_message;

        static size_t encoded_size(const discovery_message_t& message) 
        {
            const size_t message_header_size = header_codec_t::size();
            const size_t message_size = s_node_id_size + s_nodes_array_size + 
                (s_nodes_array_element_size * message.m_nodes.size());

            size_t message_dynamic_size = 0;
            for (const node_t& node : message.m_nodes)
            {
                message_dynamic_size += node.m_jumps.size() * sizeof(uint16_t);
            }
            
            return message_header_size + message_size + message_dynamic_size;
        }

        static bool encode(network::buffer_t& buffer, const discovery_message_t& message)
        {
            const size_t message_size = encoded_size(message);
            // don't touch the buffer if we cant fit the whole message inside
            if (buffer.remaining_bytes() < message_size)
            {
                return false;
            }
            
            header_codec_t::encode(buffer, message_size, s_message_type);

            buffer.append(boost::endian::native_to_big((uint16_t)message.m_identifier));
            buffer.append(boost::endian::native_to_big((uint16_t)message.m_nodes.size()));

            for (const node_t& node : message.m_nodes)
            {
                buffer.append(boost::endian::native_to_big((uint16_t)node.m_identifier));
                buffer.append(boost::endian::native_to_big((uint32_t)node.m_address.to_uint()));
                buffer.append(boost::endian::native_to_big((uint16_t)node.m_jumps.size()));
                for (const uint16_t jump : node.m_jumps)
                {
                    buffer.append(boost::endian::native_to_big((uint16_t)jump));
                }
            }

            return true;
        }

        static expected_discovery_message_t decode(network::buffer_t& buffer)
        {
            if (buffer.bytes_left_to_read() < s_node_id_size + s_nodes_array_size)
            {
                return std::unexpected("not enough bytes in buffer to read discovery_message.");
            }

            uint16_t identifier = boost::endian::big_to_native(buffer.read_uint16());
            uint16_t size = boost::endian::big_to_native(buffer.read_uint16());
            
            discovery_message_t result{.m_identifier = identifier};

            for (uint16_t node_nr = 0; node_nr < size; ++node_nr)
            {
                uint16_t node_id = boost::endian::big_to_native(buffer.read_uint16());
                uint32_t ip_address = boost::endian::big_to_native(buffer.read_uint32());

                node_t node{.m_identifier = node_id, .m_address = discnet::address_v4_t(ip_address)};

                uint16_t jumps = boost::endian::big_to_native(buffer.read_uint16());
                for (uint16_t jump_nr = 0; jump_nr < jumps; ++jump_nr)
                {
                    uint16_t jump = boost::endian::big_to_native(buffer.read_uint16());
                    node.m_jumps.push_back(jump);
                }

                result.m_nodes.push_back(node);
            }

            return result;
        }
    };
} // namespace discnet::network::messages