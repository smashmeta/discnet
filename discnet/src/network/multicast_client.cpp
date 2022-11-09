/*
 *
 */

/// data_handler
#include <mutex>
#include <boost/circular_buffer.hpp>
#include <discnet/network/messages/packet.hpp>
#include <discnet/network/buffer.hpp>
#include <discnet/network/read_buffer.hpp>
/// !data_handler

#include <discnet/network/multicast_client.hpp>


namespace discnet::network
{
    class data_handler
    {
        data_handler(size_t buffer_size)
            : m_buffer(buffer_size)
        {
            // nothing for now
        }

        void process()
        {
            std::scoped_lock lock{m_mutex};

            // size_t buffer_size = m_buffer.bytes_left_to_read();
            // if (buffer_size >= messages::packet_codec_t::s_header_size)
            // {
            //     size_t packet_size = network_to_native(m_buffer.read_int32());
            //     if (buffer_size >= packet_size)
            //     {
            //         messages::expected_packet_t packet = messages::packet_codec_t::decode(m_buffer);
            //         if (packet.has_value())
            //         {
            // 
            //         }
            //     }
            // }
        }

        void handle_receive(boost::asio::const_buffer data)
        {
            std::scoped_lock lock{m_mutex};

            size_t new_unread_size = data.size() + m_buffer.size();
            if (new_unread_size > m_buffer.capacity())
            {
                size_t new_buffer_size = new_unread_size * 2;

                boost::circular_buffer<discnet::byte_t> new_buffer(new_buffer_size);
                std::copy(m_buffer.begin(), m_buffer.begin() + m_buffer.size(), std::back_inserter(new_buffer));
                m_buffer = new_buffer;
            }

            const discnet::byte_t* data_ptr = reinterpret_cast<const discnet::byte_t*>(data.data());
            std::copy(data_ptr, data_ptr + data.size() * sizeof(discnet::byte_t), std::back_inserter(m_buffer));
        }
    private:
        boost::circular_buffer<discnet::byte_t> m_buffer;
        std::mutex m_mutex;
    };

    multicast_client::multicast_client(discnet::shared_io_service io_service, multicast_info info, size_t buffer_size)
        :   m_service(io_service), 
            m_rcv_socket(new discnet::socket_t{*io_service.get()}),
            m_snd_socket(new discnet::socket_t{*io_service.get()}),
            m_rcv_buffer(buffer_size),
            m_info(info)
    {
        // nothing for now
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
        // todo: implement
        boost::ignore_unused(error);
        boost::ignore_unused(bytes_received);

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