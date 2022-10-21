/*
 *
 */

#pragma once

#include <vector>
#include <discnet_lib/node.hpp>
#include <discnet_lib/network/buffer.hpp>
#include <discnet_lib/network/messages/header.hpp>

namespace discnet::network::messages
{
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
        uint16_t m_id;
        nodes_vector_t m_nodes;
    };

    struct discovery_message_codec_t
    {
        static const size_t s_node_id_size = sizeof(uint16_t);
        static const size_t s_nodes_array_size = sizeof(uint32_t);
        static const size_t s_nodes_array_element_size = sizeof(uint16_t) + sizeof(uint32_t);
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
            // don't touch the buffer if we cant fit the whole message inside
            if (buffer.remaining_bytes() < encoded_size(message))
            {
                return false;
            }

            // message size set in the header section
            const size_t message_size = s_node_id_size + s_nodes_array_size + 
                (s_nodes_array_element_size * message.m_nodes.size()); 
            
            header_codec_t::encode(buffer, message_size, s_message_type);

            buffer.append(boost::endian::native_to_big((uint16_t)message.m_id));
            buffer.append(boost::endian::native_to_big((uint32_t)message.m_nodes.size()));

            for (const node_t& node : message.m_nodes)
            {
                buffer.append(boost::endian::native_to_big((uint16_t)node.m_identifier.m_id));
                buffer.append(boost::endian::native_to_big((uint32_t)node.m_identifier.m_address.to_uint()));
                for (const uint16_t jump : node.m_jumps)
                {
                    buffer.append(boost::endian::native_to_big((uint16_t)jump));
                }
            }

            return true;
        }
    };
} // namespace discnet::network::messages