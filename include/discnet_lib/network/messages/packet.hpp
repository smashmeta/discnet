/*
 *
 */

#pragma once

#include <variant>
#include <expected>
#include <boost/endian.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <discnet_lib/discnet_lib.hpp>
#include <discnet_lib/typedefs.hpp>
#include <discnet_lib/network/buffer.hpp>
#include <discnet_lib/network/messages/discovery_message.hpp>

namespace discnet::network::messages
{
    using message_variant_t = std::variant<discovery_message_t>;
    using message_list_t = std::vector<message_variant_t>;

    struct packet_t
    {
        message_list_t m_messages;
    };

    struct packet_codec_t
    {
        static const size_t packet_size = 4;
        static const size_t messages_count_size = 2;
        static const size_t checksum_size = 4;
        static const size_t header_size = packet_size + messages_count_size + checksum_size;
        using md5 = boost::uuids::detail::md5;

        struct visit_message_size_t
        {
            size_t operator()(const discovery_message_t& message) const
            {
                return discovery_message_codec_t::encoded_size(message);
            }
        };

        struct visit_message_encode_t
        {
            visit_message_encode_t(discnet::network::buffer_t& buffer) : m_buffer(buffer){}
            discnet::network::buffer_t& m_buffer;
            bool operator()(const discovery_message_t& message) const
            {
                return discovery_message_codec_t::encode(m_buffer, message);
            }
        };

        static bool encode(discnet::network::buffer_t& buffer, const message_list_t& messages)
        {
            size_t packet_size = header_size;
            for (const message_variant_t& message : messages)
            {
                packet_size += std::visit(visit_message_size_t(), message);
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
} // !namespace discnet::network::messages