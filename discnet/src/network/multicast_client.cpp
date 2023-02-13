/*
 *
 */

/// data_handler
#include <mutex>
#include <discnet/network/messages/packet.hpp>
#include <discnet/network/buffer.hpp>
#include <discnet/network/read_buffer.hpp>
/// !data_handler

#include <discnet/network/multicast_client.hpp>


namespace discnet::network
{
    struct data_stream_identifier
    {
        discnet::address_v4_t::uint_type m_sender_ip = discnet::init_required;
        discnet::address_v4_t::uint_type m_recipient_ip = discnet::init_required;

        bool operator<(const data_stream_identifier& rhs) const
        {
            return  (m_sender_ip < rhs.m_sender_ip) || 
                    (m_sender_ip == rhs.m_sender_ip && m_recipient_ip < rhs.m_recipient_ip);
        }
    };

    class data_stream
    {
    public:
        data_stream(size_t buffer_size)
            :   m_buffer(buffer_size), 
                m_inital_receive(std::chrono::system_clock::now()),  
                m_last_received(m_inital_receive)
        {
            // nothing for now
        }

        data_stream(const data_stream& stream)
           :    m_buffer(stream.m_buffer),
                m_inital_receive(stream.m_inital_receive),
                m_last_received(stream.m_last_received)
        {
          // dummy
        }

        std::vector<messages::packet_t> process()
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

        void handle_receive(boost::asio::const_buffer data)
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
        
        discnet::time_point_t m_inital_receive;
        discnet::time_point_t m_last_received;
        std::vector<discnet::byte_t> m_buffer;
        std::mutex m_mutex;
    };

    class data_handler
    {
    public:
        data_handler(size_t buffer_size)
            : m_buffer_size(buffer_size)
        {
            // nothing for now
        }
        
        std::vector<messages::packet_t> process()
        {
            std::scoped_lock lock{ m_mutex };
            std::vector<messages::packet_t> result;
            return result;
        }

        void handle_receive(boost::asio::const_buffer data, const discnet::address_v4_t& sender, const discnet::address_v4_t& recipient)
        {
            std::scoped_lock lock{ m_mutex };

            data_stream_identifier identifier{ sender.to_uint(), recipient.to_uint() };
            auto itr_stream = m_streams.find(identifier);
            if (itr_stream == m_streams.end())
            {
                auto result = m_streams.emplace(identifier, std::move(data_stream(m_buffer_size)));
                itr_stream = result.first;

                if (itr_stream == m_streams.end())
                {
                    // todo: report error. failed to add new stream entry
                    return;
                } 
            }

            itr_stream->second.handle_receive(data);
        }
    private:
        std::map<data_stream_identifier const, data_stream> m_streams;
        size_t m_buffer_size;
        std::mutex m_mutex;
    };
    
    multicast_client::multicast_client(discnet::shared_io_service io_service, multicast_info info, size_t buffer_size)
        :   m_data_handler(new data_handler(4095)),
            m_service(io_service), 
            m_rcv_socket(new discnet::socket_t{*io_service.get()}),
            m_snd_socket(new discnet::socket_t{*io_service.get()}),
            m_rcv_buffer(buffer_size),
            m_info(info)
    {
        // nothing for now
    }

    void multicast_client::process()
    {
        if (!m_rcv_endpoint.address().is_v4())
        {
            // only supporting ipv4 for now
            return;
        }

        network_info_t info;
        info.m_adapter = m_info.m_adapter_address;
        info.m_receiver = m_info.m_multicast_address;
        info.m_sender = m_rcv_endpoint.address().to_v4();
        info.m_reception_time = std::chrono::system_clock::now();

        auto packets = m_data_handler->process();
        for (const messages::packet_t& packet : packets)
        {
            e_packet_received(packet, info);
        }
    }

    bool multicast_client::open()
    {
        bool result = open_multicast_rcv_socket() && open_multicast_snd_socket();
        
        if (result)
        {
            boost::asio::mutable_buffer data_buffer((void*)m_rcv_buffer.data(), m_rcv_buffer.size());
            m_rcv_socket->async_receive_from(data_buffer, m_rcv_endpoint, 
                boost::bind(&multicast_client::handle_read, this, 
                    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        }
        
        if (!result)
        {
            close();
        }

        return result;
    }

    bool multicast_client::write(const discnet::network::buffer_t& buffer)
    {
        using udp_t = boost::asio::ip::udp;
        
        discnet::error_code_t error;
        auto const_buffer = boost::asio::const_buffer(buffer.data().data(), buffer.data().size());
        udp_t::endpoint multicast_endpoint{m_info.m_multicast_address, m_info.m_multicast_port};
        m_snd_socket->async_send_to(const_buffer, multicast_endpoint, 
            boost::bind(&multicast_client::handle_write, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        
        return true;
    }

    void multicast_client::handle_write(const boost::system::error_code& error, size_t bytes_transferred)
    {
        // todo: implement
        boost::ignore_unused(error);
        boost::ignore_unused(bytes_transferred);
    }

    void multicast_client::handle_read(const boost::system::error_code& error, size_t bytes_received)
    {
        if (!error)
        {
            m_data_handler->handle_receive(
                boost::asio::const_buffer(m_rcv_buffer.data(), bytes_received), 
                m_rcv_endpoint.address().to_v4(), m_info.m_multicast_address);
        }
        else
        {
            // todo: write error message if handle_read receives an error
        }

        boost::asio::mutable_buffer data_buffer((void*)m_rcv_buffer.data(), m_rcv_buffer.size());
            m_rcv_socket->async_receive_from(data_buffer, m_rcv_endpoint, 
                boost::bind(&multicast_client::handle_read, this, 
                    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void multicast_client::close()
    {
        m_rcv_socket->close();
        m_snd_socket->close();
    }

    bool multicast_client::open_multicast_snd_socket()
    {
        using udp_t = boost::asio::ip::udp;
        namespace multicast = boost::asio::ip::multicast;

        discnet::error_code_t error;
        udp_t::endpoint multicast_endpoint{m_info.m_multicast_address, m_info.m_multicast_port};
        
        m_snd_socket->open(multicast_endpoint.protocol(), error);
        m_snd_socket->set_option(multicast::outbound_interface(m_info.m_adapter_address), error);

        return !error.failed();
    }

    bool multicast_client::open_multicast_rcv_socket()
    {
        using udp_t = boost::asio::ip::udp;
        namespace multicast = boost::asio::ip::multicast;

        discnet::error_code_t error;
        udp_t::endpoint multicast_endpoint{m_info.m_multicast_address, m_info.m_multicast_port};

        m_rcv_socket->open(multicast_endpoint.protocol(), error);
        m_rcv_socket->set_option(udp_t::socket::reuse_address(true), error);
        m_rcv_socket->set_option(multicast::enable_loopback(false), error);
        m_rcv_socket->bind(multicast_endpoint, error);
        m_rcv_socket->set_option(multicast::join_group(m_info.m_multicast_address, m_info.m_adapter_address), error);

        return !error.failed();
    }
} // !namespace discnet::network