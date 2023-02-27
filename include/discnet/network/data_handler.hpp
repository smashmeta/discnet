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
    class data_handler
    {
    public:
        data_handler(size_t buffer_size);
        
        std::vector<messages::packet_t> process();
        void handle_receive(boost::asio::const_buffer data, 
            const discnet::address_t& sender, const discnet::address_t& recipient);
    private:
        std::map<data_stream_identifier, data_stream> m_streams;
        size_t m_buffer_size;
        std::mutex m_mutex;
    };
}