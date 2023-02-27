/* 
 *
 */

#include <thread>
#include <mutex>
#include <discnet/network/messages/packet.hpp>
#include <discnet/network/buffer.hpp>
#include <discnet/network/read_buffer.hpp>
#include <discnet/network/data_stream.hpp>


namespace discnet::network
{
    bool data_stream_identifier::operator<(const data_stream_identifier& rhs) const
    {
        return  (m_sender_ip < rhs.m_sender_ip) || 
                (m_sender_ip == rhs.m_sender_ip && m_recipient_ip < rhs.m_recipient_ip);
    }

    data_stream::data_stream(size_t buffer_size)
        :   m_buffer(buffer_size), 
            m_inital_receive(std::chrono::system_clock::now()),  
            m_last_received(m_inital_receive)
    {
        // nothing for now
    }

    std::vector<messages::packet_t> data_stream::process()
    {
        std::scoped_lock lock{ m_mutex };
        std::vector<messages::packet_t> result;

        bool read_more = true;
        while (read_more)
        {
            // todo: support read_buffer_t -> requires rewrite of *::decode(...)
            //  Currently copying buffer each time we call process() which is not ideal.
            discnet::network::buffer_t buffer(m_buffer.size());
            buffer.append(buffer_span_t(m_buffer.begin(), m_buffer.size()));

            size_t buffer_size = buffer.bytes_left_to_read();
            if (buffer_size >= messages::packet_codec_t::s_header_size)
            {
                size_t packet_size = network_to_native(buffer.read_int32());
                if (buffer_size >= packet_size)
                {
                    messages::expected_packet_t packet = messages::packet_codec_t::decode(buffer);
                    if (packet.has_value())
                    {
                        result.push_back(packet.value());
                    }
                    else
                    {
                        // invalid packet? write warning!
                        m_buffer.clear();
                        read_more = false;
                    }

                    // update read buffer
                    size_t remaining_bytes = buffer.bytes_left_to_read();
                    if (remaining_bytes >= buffer.size())
                    {
                        // we did not read any data? what to do?
                        read_more = false;
                    }
                    else if (remaining_bytes == 0)
                    {
                        // all bytes have been read
                        m_buffer.clear();
                        read_more = false;
                    }
                    else
                    {
                        // partial buffer read
                        size_t bytes_read = buffer.size() - remaining_bytes;
                        std::shift_left(m_buffer.begin(), m_buffer.end(), bytes_read);
                        m_buffer.resize(remaining_bytes);
                    }
                }
                else
                {
                    // incomplete packet
                    read_more = false;
                }
            }
        }

        return result;
    }

    void data_stream::handle_receive(boost::asio::const_buffer data)
    {
        std::scoped_lock lock{ m_mutex };

        size_t new_unread_size = data.size() + m_buffer.size();
        if (new_unread_size > m_buffer.capacity())
        {
            size_t new_buffer_size = new_unread_size * 2;
            std::vector<discnet::byte_t> new_buffer(new_buffer_size);
            std::copy(m_buffer.begin(), m_buffer.begin() + m_buffer.size(), std::back_inserter(new_buffer));
            m_buffer = new_buffer;
        }

        const discnet::byte_t* data_ptr = reinterpret_cast<const discnet::byte_t*>(data.data());
        std::copy(data_ptr, data_ptr + data.size() * sizeof(discnet::byte_t), std::back_inserter(m_buffer));
        m_last_received = std::chrono::system_clock::now();
    }
} // !namespace discnet::network