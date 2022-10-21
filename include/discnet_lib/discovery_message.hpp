/*
 *
 */

#pragma once

#include <expected>
#include <variant>
#include <boost/endian.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <discnet_lib/network/buffer.hpp>
#include <discnet_lib/node.hpp>
#include <discnet_lib/typedefs.hpp>
#include <discnet_lib/discnet_lib.hpp>

namespace discnet
{
    namespace
    {
        template<class> inline constexpr bool always_false_v = false;
    } // !anonymouse namespace

    enum class message_type_e : uint16_t
    {
        discovery_message = 1
    };

    /*
    header [
        size: 24,
        type: discovery_message (1)
    ]
    */
    struct message_header_t
    {
        uint32_t m_size;
        message_type_e m_type;
    };

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
        typedef std::vector<node_identifier_t> nodes_vector_t;

        uint16_t m_id;
        nodes_vector_t m_nodes;
    };

    using message_variant_t = std::variant<discnet::discovery_message_t>;
    using message_list_t = std::vector<message_variant_t>;

    /*
    buffer [
        message_header_t + message_*_t bytes
    ]
    */
    struct encoded_message_t
    {
        std::vector<discnet::byte_t> m_buffer;
    };

    /*
    packet [
        messages: 3,
        size_1: n,
        [message_1],
        size_2: n,
        [message_2],
        size_3: n,
        [message_3],
        checksum: n
    ]
    */
    struct packet_t
    {
        std::vector<encoded_message_t> m_messages;
    };

    /*
    info [
        time: now(),
        sender: 192.200.1.10
        receiver: 238.200.200.200
        adapter: 192.200.1.2
    ]
    */
    struct network_info_t
    {
        time_point_t m_reception_time;
        address_v4_t m_sender;
        address_v4_t m_receiver;
        address_v4_t m_adapter;
    };

    struct discovery_data_handler_t
    {
        boost::signals2::signal<void(const discovery_message_t&, const network_info_t&)> e_discovery_message;
        
        bool process(std::span<discnet::byte_t> packet, const network_info_t& packet_info)
        {
            // todo: decode data packet into a discovery message.
            return false;
        }
    };

    struct packet_codec_t
    {
        static const size_t packet_size = 4;
        static const size_t messages_count_size = 2;
        static const size_t checksum_size = 4;
        static const size_t header_size = packet_size + messages_count_size + checksum_size;
        using md5 = boost::uuids::detail::md5;

        static size_t message_size_visit(const message_variant_t& message_variant)
        {
            return std::visit(
                [&](const auto& message) -> size_t
                {
                    using message_type = std::decay_t<decltype(message)>;
                    if constexpr (std::is_same_v<message_type, discovery_message_t>)
                    {
                        const discovery_message_t& discovery_msg = (discovery_message_t&)message;
                        return discovery_message_codec_t::encoded_size(discovery_msg); 
                    }
                    else
                    {
                        static_assert(always_false_v, "non-exhaustive visitor!");
                    }
                }
            ,message_variant);
        }

        static bool message_encode_visit(discnet::network::buffer_t& buffer, const message_variant_t& message_variant)
        {
            return std::visit(
                [&](const auto& message) -> bool
                {
                    using message_type = std::decay_t<decltype(message)>;
                    if constexpr (std::is_same_v<message_type, discovery_message_t>)
                    {
                        const discovery_message_t& discovery_msg = (discovery_message_t&)message;
                        return discovery_message_codec_t::encode(buffer ,discovery_msg); 
                    }
                    else
                    {
                        static_assert(always_false_v, "non-exhaustive visitor!");
                    }
                }
            ,message_variant);
        }

        static bool encode(discnet::network::buffer_t& buffer, const message_list_t& messages)
        {
            size_t packet_size = header_size;
            for (const message_variant_t& message : messages)
            {
                packet_size += message_size_visit(message);
            }

            // verify that we can fit the packet inside our buffer
            if (packet_size > buffer.remaining_bytes())
            {
                // todo: error message goes here.
                return false;
            }

            buffer.append(boost::endian::native_to_big((uint32_t)packet_size));
            buffer.append(boost::endian::native_to_big((uint16_t)messages.size()));

            for (const message_variant_t& message : messages)
            {
                if (!message_encode_visit(buffer, message))
                {
                    // todo: error message goes here.
                    return false;
                }
            }

            // generate hash for message
            md5 hash;
            md5::digest_type digest;
            hash.process_bytes((const void*)(buffer.data().data()), buffer.appended_bytes());
            hash.get_digest(digest);

            // only using the last 4 bytes for verification
            buffer.append(boost::endian::native_to_big((uint32_t)digest[3]));

            return true;
        }

        static std::vector<discnet::byte_t> encode(discnet::network::buffer_span_t messages_buffer, size_t message_count)
        {
            size_t payload_size = messages_buffer.size();
            std::vector<discnet::byte_t> result(header_size + payload_size, 0);
            (uint32_t&)result[0] = boost::endian::native_to_big((uint32_t)(header_size + payload_size));
            (uint16_t&)result[4] = boost::endian::native_to_big((uint16_t)message_count);
            size_t current_index = header_size - checksum_size;
            std::copy(messages_buffer.begin(), messages_buffer.end(), result.begin() + current_index);
            current_index += payload_size;

            md5 hash;
            md5::digest_type digest;
            size_t bytes_to_checksum_size = payload_size + header_size - checksum_size;
            hash.process_bytes((const void*)&(result[0]), bytes_to_checksum_size);
            hash.get_digest(digest);
            
            // only using the last 4 bytes for verification
            (uint32_t&)result[current_index] = boost::endian::native_to_big((uint32_t)digest[3]);

            return result;
        }

        static std::vector<discnet::byte_t> encode(const packet_t& packet)
        {
            size_t payload_size = 0;
            for (const auto& message : packet.m_messages)
            {
                payload_size += message.m_buffer.size();
            }

            std::vector<discnet::byte_t> result(header_size + payload_size, 0);
            (uint32_t&)result[0] = boost::endian::native_to_big((uint32_t)(header_size + payload_size));
            (uint16_t&)result[4] = boost::endian::native_to_big((uint16_t)packet.m_messages.size());
            
            size_t current_index = header_size - checksum_size;
            for (const auto& message : packet.m_messages)
            {
                std::copy(message.m_buffer.begin(), message.m_buffer.end(), result.begin() + current_index);
                current_index += message.m_buffer.size();
            }

            md5 hash;
            md5::digest_type digest;
            hash.process_bytes((const void*)&(result[0]), payload_size + header_size - checksum_size);
            hash.get_digest(digest);

            // only using the last 4 bytes for verification
            (uint32_t&)result[current_index] = boost::endian::native_to_big((uint32_t)digest[3]);

            return result;
        }

        static std::expected<packet_t, std::string> decode(const std::span<discnet::byte_t>& buffer)
        {

        }
    };

    struct header_codec_t
    {
        const static size_t s_header_size = 6;

        static size_t size()
        {
            return s_header_size;
        }

        static bool encode(network::buffer_t& buffer, uint32_t size, message_type_e message_type)
        {
            if (buffer.remaining_bytes() < s_header_size)
            {
                return false;
            }

            buffer.append(boost::endian::native_to_big((uint32_t)size));
            buffer.append(boost::endian::native_to_big((uint16_t)message_type));

            return true;
        }

        static std::vector<discnet::byte_t> encode(const message_header_t& message)
        {
            std::vector<discnet::byte_t> result(s_header_size, 0);
            (uint32_t&)result[0] = boost::endian::native_to_big(message.m_size);
            (uint16_t&)result[4] = boost::endian::native_to_big((uint16_t)message.m_type);
            return result;
        }

        static std::expected<message_header_t, std::string> decode(const std::span<discnet::byte_t>& buffer)
        {
            if (buffer.size() != s_header_size)
            {
                return std::unexpected("invalid header size.");
            }

            message_header_t header;
            header.m_size = boost::endian::big_to_native((uint32_t&)buffer[0]);
            header.m_type = (message_type_e)boost::endian::big_to_native((uint16_t&)buffer[4]);
            return header;
        }
    };

    struct discovery_message_codec_t
    {
        static const size_t s_node_id_size = 2;
        static const size_t s_nodes_array_size = 4;
        static const size_t s_nodes_array_element_size = 6;
        static const message_type_e s_message_type = message_type_e::discovery_message;

        static size_t encoded_size(const discovery_message_t& message) 
        {
            const size_t message_header_size = header_codec_t::size();
            const size_t message_size = s_node_id_size + s_nodes_array_size + 
                (s_nodes_array_element_size * message.m_nodes.size());

            return message_header_size + message_size;
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

            for (const node_identifier_t& node : message.m_nodes)
            {
                buffer.append(boost::endian::native_to_big((uint16_t)node.m_id));
                buffer.append(boost::endian::native_to_big((uint32_t)node.m_address.to_uint()));
            }

            return true;
        }

        static std::vector<discnet::byte_t> encode(const discovery_message_t& message)
        {
            const size_t nodes_array_total_size = message.m_nodes.size() * s_nodes_array_element_size;
            const size_t buffer_size = s_node_id_size + s_nodes_array_size + nodes_array_total_size;
            std::vector<discnet::byte_t> result(buffer_size, 0);
            
            (uint16_t&)result[0] = boost::endian::native_to_big(message.m_id);
            (uint32_t&)result[2] = boost::endian::native_to_big((uint32_t)message.m_nodes.size());

            size_t index = 0;
            for (const node_identifier_t& node : message.m_nodes)
            {
                size_t current_index = (index * s_nodes_array_element_size) + 6;
                (uint16_t&)result[current_index] = boost::endian::native_to_big(node.m_id);
                current_index += 2;
                (uint32_t&)result[current_index] = boost::endian::native_to_big(node.m_address.to_uint());
                index += 1;
            }

            return result;
        }

        static std::expected<discovery_message_t, std::string> decode(const std::span<discnet::byte_t>& buffer)
        {
            // verify buffer size first
            size_t buffer_size = buffer.size();
            if (buffer_size < 6)
            {
                return std::unexpected("invalid buffer size. header size invalid.");
            }

            discovery_message_t message;
            message.m_id = (uint16_t&)buffer[0];
            boost::endian::big_to_native_inplace(message.m_id);

            uint32_t elements = (uint32_t&)buffer[2];
            boost::endian::big_to_native_inplace(elements);

            if ((buffer_size - 6) % s_nodes_array_element_size != 0)
            {
                return std::unexpected("invalid buffer size. elements size invalid.");
            }

            size_t array_elements = (buffer_size - 6) / s_nodes_array_element_size;
            if (array_elements != elements)
            {
                return std::unexpected("mismatch between reported elements count and remaining bytes to parse.");
            }

            for (size_t index = 0; index < array_elements; ++index)
            {
                size_t current_index = (index * s_nodes_array_element_size) + 6;
                node_identifier_t node;
                node.m_id = (uint16_t&)buffer[current_index];
                boost::endian::big_to_native_inplace(node.m_id);
                current_index += 2;
                uint32_t node_address = (uint32_t&)(buffer[current_index]);
                node.m_address = address_v4_t(boost::endian::big_to_native(node_address));

                message.m_nodes.push_back(node);
            }
 
            return message;
        }
    };
} // !namespace discnet