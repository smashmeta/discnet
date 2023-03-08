/*
 *
 */

#include <whatlog/logger.hpp>
#include <discnet/network/multicast_client.hpp>


namespace discnet::network
{
    shared_multicast_client multicast_client::create(discnet::shared_io_service io_service, multicast_info_t info, shared_data_handler data_handler) 
    {
        return std::shared_ptr<multicast_client>(new multicast_client{io_service, info, data_handler});
    }

    multicast_client::multicast_client(discnet::shared_io_service io_service, multicast_info_t info, shared_data_handler data_handler)
        :   m_data_handler(data_handler),
            m_service(io_service), 
            m_rcv_socket(new discnet::socket_t{*io_service.get()}),
            m_snd_socket(new discnet::socket_t{*io_service.get()}),
            m_rcv_buffer(12560, '\0'),
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
                boost::bind(&multicast_client::handle_read, shared_from_this(), 
                    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        }
        else
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
            boost::bind(&multicast_client::handle_write, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        
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
        whatlog::logger log("multicast_client::handle_read");
        if (!error)
        {
            m_data_handler->handle_receive(
                boost::asio::const_buffer(m_rcv_buffer.data(), bytes_received), 
                m_rcv_endpoint.address().to_v4(), m_info.m_multicast_address);
        }
        else
        {
            if (error == boost::asio::error::connection_aborted)
            {
                log.info("closing multicast connection on adapter {}.", m_info.m_adapter_address.to_string());
                close();
            }
            else
            {
                log.warning("error reading data. id: {}, message: {}.", error.value(), error.message());
            }
            
            return;
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

    multicast_info_t multicast_client::info() const
    {
        return m_info;
    }

    bool multicast_client::open_multicast_snd_socket()
    {
        using udp_t = boost::asio::ip::udp;
        namespace multicast = boost::asio::ip::multicast;
        whatlog::logger log("open_multicast_snd_socket");

        discnet::error_code_t error;
        udp_t::endpoint multicast_endpoint{m_info.m_multicast_address, m_info.m_multicast_port};
        
        m_snd_socket->open(multicast_endpoint.protocol(), error);
        if (error.failed())
        {
            log.warning("failed to open socket on local address: {}, port: {}. Error: {}.", 
                multicast_endpoint.address().to_string(), multicast_endpoint.port(), error.message());
            return false;
        }

        m_snd_socket->set_option(multicast::outbound_interface(m_info.m_adapter_address), error);
        if (error.failed())
        {
            log.warning("failed to enable udp socket option: outbound_interface with address: {}. Error: {}.", 
                m_info.m_adapter_address.to_string(), error.message());
        }

        return !error.failed();
    }

    bool multicast_client::open_multicast_rcv_socket()
    {
        using udp_t = boost::asio::ip::udp;
        namespace multicast = boost::asio::ip::multicast;
        whatlog::logger log("open_multicast_rcv_socket");

        log.info("setting up multicast listening socket - addr: {}, port: {}, on adapter {}.", 
            m_info.m_multicast_address.to_string(),
            m_info.m_multicast_port,
            m_info.m_adapter_address.to_string());

        udp_t::endpoint multicast_endpoint{discnet::address_t::any(), m_info.m_multicast_port};
        
        discnet::error_code_t error;
        m_rcv_socket->open(multicast_endpoint.protocol(), error);
        if (error.failed())
        {
            log.warning("failed to open socket on local address: {}, port: {}. Error: {}.", 
                multicast_endpoint.address().to_string(), multicast_endpoint.port(), error.message());
            return false;
        }
        
        m_rcv_socket->set_option(udp_t::socket::reuse_address(true), error);
        if (error.failed())
        {
            log.warning("failed to enable udp socket option: reuse_address. Error: {}.", error.message());
            return false;
        }

        m_rcv_socket->set_option(multicast::enable_loopback(false), error);
        if (error.failed())
        {
            log.warning("failed to enable udp socket option: disable_loopback. Error: {}.", error.message());
            return false;
        }
        
        m_rcv_socket->bind(multicast_endpoint, error);
        if (error.failed())
        {
            log.warning("failed to bind socket on local address: {}, port: {}. Error: {}.", 
                multicast_endpoint.address().to_string(), multicast_endpoint.port(), error.message());
            return false;
        }

        m_rcv_socket->set_option(multicast::join_group(m_info.m_multicast_address, m_info.m_adapter_address), error);
        if (error.failed())
        {
            log.warning("failed to enable udp socket option: join_group. multicast-address: {}, adapter-address: {}. Error: {}.", 
                m_info.m_multicast_address.to_string(), m_info.m_adapter_address.to_string(), error.message());
            return false;
        }

        return !error.failed();
    }
} // !namespace discnet::network