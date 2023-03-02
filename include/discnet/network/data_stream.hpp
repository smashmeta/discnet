/*
 *
 */

#pragma once

#include <discnet/typedefs.hpp>
#include <discnet/network/messages/packet.hpp>

namespace discnet::network
{
    struct data_stream_identifier
    {
        discnet::address_t m_sender_ip = discnet::init_required;
        discnet::address_t m_recipient_ip = discnet::init_required;

        DISCNET_EXPORT bool operator<(const data_stream_identifier& rhs) const;
    };

    class data_stream
    {
    public:
        DISCNET_EXPORT data_stream(size_t buffer_size);

        data_stream() = delete;
        data_stream(const data_stream& stream) = delete;
        data_stream(data_stream&& stream) = delete;

        DISCNET_EXPORT std::vector<messages::packet_t> process();
        DISCNET_EXPORT void handle_receive(boost::asio::const_buffer data);
        
        discnet::time_point_t m_inital_receive;
        discnet::time_point_t m_last_received;
        std::vector<discnet::byte_t> m_buffer;
        std::mutex m_mutex;
    };
} // namespace discnet::network