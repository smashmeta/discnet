/*
 *
 */

#include <discnet/network/data_handler.hpp>


namespace discnet::network
{
    data_handler::data_handler(size_t buffer_size)
        : m_buffer_size(buffer_size)
    {
        // nothing for now
    }
    
    std::vector<messages::packet_t> data_handler::process()
    {
        std::scoped_lock lock{ m_mutex };
        std::vector<messages::packet_t> result;

        for (auto& [identifier, stream] : m_streams)
        {
            result = stream.process();
        }

        return result;
    }

    void data_handler::handle_receive(boost::asio::const_buffer data, const discnet::address_t& sender, const discnet::address_t& recipient)
    {
        std::scoped_lock lock{ m_mutex };

        data_stream_identifier identifier{ sender, recipient };
        auto itr_stream = m_streams.find(identifier);
        if (itr_stream == m_streams.end())
        {
            auto result = m_streams.try_emplace(identifier, m_buffer_size);
            itr_stream = result.first;

            if (itr_stream == m_streams.end())
            {
                // todo: report error. failed to add new stream entry
                return;
            } 
        }

        itr_stream->second.handle_receive(data);
    }
} // !namespace discnet::network