/*
 *
 */

#pragma once

#include <map>
#include <ranges>
#include <mutex>
#include <discnet/network/data_stream.hpp>

namespace discnet::network
{
    struct data_stream_packets_t
    {
        data_stream_identifier m_identifier = discnet::init_required;
        std::vector<messages::packet_t> m_packets = {};
    };

    using packet_collection_t = std::vector<data_stream_packets_t>;
    using data_received_func = std::function<void (boost::asio::const_buffer, const discnet::address_t&, const discnet::address_t&)>;

    class data_handler
    {
    public:
        DISCNET_EXPORT data_handler(size_t buffer_size);
        
        DISCNET_EXPORT packet_collection_t process();
        DISCNET_EXPORT void handle_receive(boost::asio::const_buffer data, 
            const discnet::address_t& sender, const discnet::address_t& recipient);
    private:
        std::map<data_stream_identifier, data_stream> m_streams;
        size_t m_buffer_size;
        std::mutex m_mutex;
    };

    typedef std::shared_ptr<data_handler> shared_data_handler;
}