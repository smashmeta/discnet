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
#include <discnet_lib/network/messages/data_message.hpp>

namespace discnet::network::messages
{
    using message_variant_t = std::variant<discovery_message_t, data_message_t>;
    using message_list_t = std::vector<message_variant_t>;

    struct packet_t
    {
        message_list_t m_messages = {};
    };

    using expected_packet_t = std::expected<packet_t, std::string>;

    struct packet_codec_t
    {
        static const size_t s_packet_size_field_size = 4;
        static const size_t s_message_count_field_size = 2;
        static const size_t s_checksum_size = 4;
        static const size_t s_header_size = s_packet_size_field_size + s_message_count_field_size + s_checksum_size;
        using md5 = boost::uuids::detail::md5;

        struct visit_message_size_t
        {
            size_t operator()(const discovery_message_t& message) const
            {
                return discovery_message_codec_t::encoded_size(message);
            }

            size_t operator()(const data_message_t& message) const
            {
                return data_message_codec_t::encoded_size(message);
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

            bool operator()(const data_message_t& message) const
            {
                return data_message_codec_t::encode(m_buffer, message);
            }
        };

        static bool encode(discnet::network::buffer_t& buffer, const message_list_t& messages)
        {
            size_t total_packet_size = s_header_size;
            for (const message_variant_t& message : messages)
            {
                total_packet_size += std::visit(visit_message_size_t(), message);
            }

            // verify that we can fit the packet inside our buffer
            if (total_packet_size > buffer.remaining_bytes())
            {
                // todo: error message goes here.
                return false;
            }

            buffer.append(boost::endian::native_to_big((uint32_t)total_packet_size));
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

        static expected_packet_t decode(network::buffer_t& buffer)
        {
            if (buffer.bytes_left_to_read() < s_packet_size_field_size)
            {
                return std::unexpected("not enough bytes in buffer to read packet. (header missing)");
            }

            uint32_t size = boost::endian::big_to_native(buffer.read<uint32_t>());
            buffer.reset_read();
            
            // todo: validate if checksum size check is correct
            if (buffer.bytes_left_to_read() < size || size < s_checksum_size)
            {
                return std::unexpected("not enough bytes in buffer to read packet. (data missing)");
            }
            
            buffer_span_t complete_message = buffer.read_buffer(size - s_checksum_size);
            uint32_t checksum = buffer.read<uint32_t>();

            // generate hash for message
            md5 hash;
            md5::digest_type digest;
            hash.process_bytes((const void*)(complete_message.data()), complete_message.size());
            hash.get_digest(digest);

            if (checksum != digest[3])
            {
                return std::unexpected("packet checksum validation failed.");
            }

            // todo: implement the rest of the function
            return std::unexpected("unfinished");
        }
    };
} // !namespace discnet::network::messages