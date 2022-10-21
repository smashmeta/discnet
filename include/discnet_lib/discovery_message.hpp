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
        template<class ... Ts> struct overloaded : Ts ... { using Ts::operator()...; };
    } // !anonymouse namespace

    enum class message_type_e : uint16_t
    {
        discovery_message = 1,
        command_message = 2
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
    };

    struct packet_codec_t
    {
        static const size_t packet_size = 4;
        static const size_t messages_count_size = 2;
        static const size_t checksum_size = 4;
        static const size_t header_size = packet_size + messages_count_size + checksum_size;
        using md5 = boost::uuids::detail::md5;

        struct visit_message_encode_t
        {
            visit_message_encode_t(discnet::network::buffer_t& buffer) : m_buffer(buffer) {}
            bool operator()(const discovery_message_t& message) const
            {
                return discovery_message_codec_t::encode(m_buffer, message);
            }

            discnet::network::buffer_t& m_buffer;
        };

        static size_t message_size_visit(const message_variant_t& message_variant)
        {
            return std::visit(overloaded {
                [](const discovery_message_t& message) -> size_t { return discovery_message_codec_t::encoded_size(message); }
            }, message_variant);
        }

        static bool message_encode_visit(discnet::network::buffer_t& buffer, const message_variant_t& message_variant)
        {
            return std::visit(overloaded {
                [&](const discovery_message_t& message) -> bool { return discovery_message_codec_t::encode(buffer, message); }
            }, message_variant);
        }

        static size_t message_size(const discovery_message_t& message)
        {
            return discovery_message_codec_t::encoded_size(message);
        }

        static bool encode(discnet::network::buffer_t& buffer, const message_list_t& messages)
        {
            size_t packet_size = header_size;
            for (const message_variant_t& message : messages)
            {
                auto size_visitor = [&](const auto& message) -> size_t { return message_size(message); };
                packet_size += std::visit(size_visitor, message);
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
                if (!std::visit(visit_message_encode_t(buffer), message))
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

        static std::expected<packet_t, std::string> decode(const std::span<discnet::byte_t>& buffer)
        {

        }
    };
} // !namespace discnet