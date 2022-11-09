/*
 *
 */

#include <discnet/network/udp_client.hpp>


namespace discnet::network
{
    udp_client::udp_client(discnet::shared_io_service io_service, multicast_info info, size_t buffer_size)
        :   m_service(io_service), 
            m_rcv_socket(new discnet::socket_t{*io_service.get()}),
            m_snd_socket(new discnet::socket_t{*io_service.get()}),
            m_rcv_buffer(buffer_size),
            m_info(info)
    {
        // nothing for now
    }

    bool udp_client::open()
    {
        bool result = open_multicast_rcv_socket() && open_multicast_snd_socket();
        
        if (result)
        {
            boost::asio::mutable_buffer data_buffer((void*)m_rcv_buffer.data(), m_rcv_buffer.size());
            m_rcv_socket->async_receive_from(data_buffer, m_rcv_endpoint, 
                boost::bind(&udp_client::handle_read, this, 
                    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        }
        
        if (!result)
        {
            close();
        }

        return result;
    }

    bool udp_client::write(const discnet::network::buffer_t& buffer)
    {
        using udp_t = boost::asio::ip::udp;
        
        discnet::error_code_t error;
        auto const_buffer = boost::asio::const_buffer(buffer.data().data(), buffer.data().size());
        udp_t::endpoint multicast_endpoint{m_info.m_multicast_address, m_info.m_multicast_port};
        m_snd_socket->async_send_to(const_buffer, multicast_endpoint, 
            boost::bind(&udp_client::handle_write, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        
        return true;
    }

    void udp_client::handle_write(const boost::system::error_code& error, size_t bytes_transferred)
    {
        // todo: implement
        boost::ignore_unused(error);
        boost::ignore_unused(bytes_transferred);
    }

    void udp_client::handle_read(const boost::system::error_code& error, size_t bytes_received)
    {
        // todo: implement
        boost::ignore_unused(error);
        boost::ignore_unused(bytes_received);

        boost::asio::mutable_buffer data_buffer((void*)m_rcv_buffer.data(), m_rcv_buffer.size());
            m_rcv_socket->async_receive_from(data_buffer, m_rcv_endpoint, 
                boost::bind(&udp_client::handle_read, this, 
                    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void udp_client::close()
    {
        m_rcv_socket->close();
        m_snd_socket->close();
    }

    bool udp_client::open_multicast_snd_socket()
    {
        using udp_t = boost::asio::ip::udp;
        namespace multicast = boost::asio::ip::multicast;

        discnet::error_code_t error;
        udp_t::endpoint multicast_endpoint{m_info.m_multicast_address, m_info.m_multicast_port};
        
        m_snd_socket->open(multicast_endpoint.protocol(), error);
        m_snd_socket->set_option(multicast::outbound_interface(m_info.m_adapter_address), error);

        return !error.failed();
    }

    bool udp_client::open_multicast_rcv_socket()
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