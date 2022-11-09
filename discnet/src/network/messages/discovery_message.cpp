/*
 *
 */

#include <discnet/network/messages/discovery_message.hpp>

namespace discnet::network::messages
{
    size_t discovery_message_codec_t::encoded_size(const discovery_message_t& message) 
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

    bool discovery_message_codec_t::encode(network::buffer_t& buffer, const discovery_message_t& message)
    {
        const size_t message_size = encoded_size(message);
        // don't touch the buffer if we cant fit the whole message inside
        if (buffer.remaining_bytes() < message_size)
        {
            return false;
        }
        
        header_codec_t::encode(buffer, message_size, s_message_type);

        buffer.append(native_to_network((uint16_t)message.m_identifier));
        buffer.append(native_to_network((uint16_t)message.m_nodes.size()));

        for (const node_t& node : message.m_nodes)
        {
            buffer.append(native_to_network((uint16_t)node.m_identifier));
            buffer.append(native_to_network((uint32_t)node.m_address.to_uint()));
            buffer.append(native_to_network((uint16_t)node.m_jumps.size()));
            for (const uint16_t jump : node.m_jumps)
            {
                buffer.append(native_to_network((uint16_t)jump));
            }
        }

        return true;
    }

    expected_discovery_message_t discovery_message_codec_t::decode(network::buffer_t& buffer)
    {
        if (buffer.bytes_left_to_read() < s_node_id_size + s_nodes_array_size)
        {
            return std::unexpected("not enough bytes in buffer to read discovery_message.");
        }

        uint16_t identifier = network_to_native(buffer.read_uint16());
        uint16_t size = network_to_native(buffer.read_uint16());
        
        discovery_message_t result{.m_identifier = identifier};

        for (uint16_t node_nr = 0; node_nr < size; ++node_nr)
        {
            uint16_t node_id = network_to_native(buffer.read_uint16());
            uint32_t ip_address = network_to_native(buffer.read_uint32());

            node_t node{.m_identifier = node_id, .m_address = discnet::address_v4_t(ip_address)};

            uint16_t jumps = network_to_native(buffer.read_uint16());
            for (uint16_t jump_nr = 0; jump_nr < jumps; ++jump_nr)
            {
                uint16_t jump = network_to_native(buffer.read_uint16());
                node.m_jumps.push_back(jump);
            }

            result.m_nodes.push_back(node);
        }

        return result;
    }

    bool operator==(const node_t& lhs, const node_t& rhs)
    {
        return lhs.m_identifier == rhs.m_identifier &&
            lhs.m_address == rhs.m_address &&
            std::equal(lhs.m_jumps.begin(), lhs.m_jumps.end(), rhs.m_jumps.begin(), rhs.m_jumps.end());
    }

    bool operator==(const discovery_message_t& lhs, const discovery_message_t& rhs)
    {
        return lhs.m_identifier == rhs.m_identifier &&
            std::equal(lhs.m_nodes.begin(), lhs.m_nodes.end(), rhs.m_nodes.begin(), rhs.m_nodes.end());
    }
} // ! namespace discnet::network::messages