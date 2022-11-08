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

        static std::expected<bool,std::string> validate_packet(network::buffer_t& buffer)
        {
            // checking that the packet header content is correct
            if (buffer.bytes_left_to_read() < s_packet_size_field_size)
            {
                return std::unexpected("not enough bytes in buffer to read packet. (header missing)");
            }

            uint32_t size = boost::endian::big_to_native(buffer.read_uint32());
            buffer.reset_read();
            
            // check that we got enough bytes in our buffer
            if (buffer.bytes_left_to_read() < size)
            {
                return std::unexpected("not enough bytes in buffer to read packet. (data missing)");
            }

            // validate our packet bytes against the given checksum digest
            buffer_span_t complete_message = buffer.read_buffer(size - s_checksum_size);
            uint32_t checksum = boost::endian::big_to_native(buffer.read_uint32());

            // generate hash for message
            md5 hash;
            md5::digest_type digest;
            hash.process_bytes((const void*)(complete_message.data()), complete_message.size());
            hash.get_digest(digest);

            if (checksum != digest[3])
            {
                return std::unexpected(std::format(
                    "packet checksum validation failed. encoded digest: {}, calculated digest: {}",
                    checksum, digest[3]));
            }

            return true;
        }

        static expected_packet_t decode(network::buffer_t& buffer)
        {
            // check that our packet is valid first
            auto valid = validate_packet(buffer);
            buffer.reset_read();

            if (!valid.has_value())
            {
                return std::unexpected(valid.error());
            }

            // fetch messages received
            size_t size = boost::endian::big_to_native(buffer.read_uint32());
            boost::ignore_unused(size);
            uint16_t messages = boost::endian::big_to_native(buffer.read_uint16());

            packet_t result;

            for (size_t i = 0; i < messages; ++i)
            {
                auto header = header_codec_t::decode(buffer);
                if (!header.has_value())
                {
                    // early exist because we do not know what to do with the
                    // rest of the buffer if we get an invalid message
                    std::string error_message = std::format("message header number {} failed to parse: {}.", i, header.error());
                    return std::unexpected(error_message);
                }

                auto header_type = header.value().m_type;
                switch (header.value().m_type)
                {
                    case message_type_e::discovery_message:
                        {
                            expected_discovery_message_t message = discovery_message_codec_t::decode(buffer);
                            if (!message.has_value())
                            {
                                return std::unexpected(std::format("failed to parse message number {}. error: {}", i, message.error()));
                            }
                            
                            result.m_messages.push_back(message.value());
                            break;
                        }
                    case message_type_e::data_message:
                        {
                            expected_data_message_t message = data_message_codec_t::decode(buffer);
                            if (!message.has_value())
                            {
                                return std::unexpected(std::format("failed to parse message number {}. error: {}", i, message.error()));
                            }
// 
                            result.m_messages.push_back(message.value());
                            break;
                        }
                        
                    default:
                        return std::unexpected(std::format("received message of unknown message type {}.", (int)header_type));
                }
            }

            return result;
        }
    };
} // !namespace discnet::network::messages