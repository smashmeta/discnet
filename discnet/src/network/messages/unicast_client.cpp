/*
 *
 */

#include <whatlog/logger.hpp>
#include <discnet/network/unicast_client.hpp>


namespace discnet::network
{
    shared_unicast_client unicast_client::create(discnet::shared_io_service io_service, unicast_info_t info, shared_data_handler data_handler) 
    {
        return std::shared_ptr<unicast_client>(new unicast_client{io_service, info, data_handler});
    }

    unicast_client::unicast_client(discnet::shared_io_service io_service, unicast_info_t info, shared_data_handler data_handler)
        :   m_data_handler(data_handler),
            m_service(io_service), 
            m_rcv_socket(new discnet::socket_t{*io_service.get()}),
            m_snd_socket(new discnet::socket_t{*io_service.get()}),
            m_rcv_buffer(12560, '\0'),
            m_info(info)
    {
        // nothing for now
    }

    bool unicast_client::open()
    {
        bool result = open_unicast_rcv_socket() && open_unicast_snd_socket();
        if (result)
        {
            boost::asio::mutable_buffer data_buffer((void*)m_rcv_buffer.data(), m_rcv_buffer.size());
            m_rcv_socket->async_receive_from(data_buffer, m_rcv_endpoint, 
                boost::bind(&unicast_client::handle_read, shared_from_this(), 
                    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        }
        else
        {
            close();
        }

        return result;
    }

    bool unicast_client::write(const discnet::address_t& recipient, const discnet::network::buffer_t& buffer)
    {
        using udp_t = boost::asio::ip::udp;
        
        discnet::error_code_t error;
        auto const_buffer = boost::asio::const_buffer(buffer.data().data(), buffer.data().size());
        udp_t::endpoint unicast_endpoint{recipient, m_info.m_port};
        m_snd_socket->async_send_to(const_buffer, unicast_endpoint, 
            boost::bind(&unicast_client::handle_write, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        
        return true;
    }

    void unicast_client::handle_write(const boost::system::error_code& error, size_t bytes_transferred)
    {
        // todo: implement
        boost::ignore_unused(error);
        boost::ignore_unused(bytes_transferred);
    }

    void unicast_client::handle_read(const boost::system::error_code& error, size_t bytes_received)
    {
        whatlog::logger log("unicast_client::handle_read");
        if (!error)
        {
            m_data_handler->handle_receive(
                boost::asio::const_buffer(m_rcv_buffer.data(), bytes_received), 
                m_rcv_endpoint.address().to_v4(), m_info.m_address);
        }
        else
        {
            if (error == boost::asio::error::connection_aborted)
            {
                log.info("closing unicast connection on adapter {}.", m_info.m_address.to_string());
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
                boost::bind(&unicast_client::handle_read, this, 
                    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void unicast_client::close()
    {
        m_rcv_socket->close();
        m_snd_socket->close();
    }

    unicast_info_t unicast_client::info() const
    {
        return m_info;
    }

    bool unicast_client::open_unicast_snd_socket()
    {
        using udp_t = boost::asio::ip::udp;
        whatlog::logger log("open_unicast_snd_socket");

        discnet::error_code_t error;
        udp_t::endpoint unicast_endpoint{m_info.m_address, m_info.m_port};
        
        m_snd_socket->open(unicast_endpoint.protocol(), error);
        if (error.failed())
        {
            log.warning("failed to open socket on local address: {}, port: {}. Error: {}.", 
                unicast_endpoint.address().to_string(), unicast_endpoint.port(), error.message());
            return false;
        }

        return !error.failed();
    }

    bool unicast_client::open_unicast_rcv_socket()
    {
        using udp_t = boost::asio::ip::udp;
        whatlog::logger log("open_unicast_rcv_socket");

        log.info("setting up unicast listening socket - addr: {}, port: {}.", 
            m_info.m_address.to_string(),
            m_info.m_port);

        udp_t::endpoint unicast_endpoint{m_info.m_address, m_info.m_port};
        
        discnet::error_code_t error;
        m_rcv_socket->open(unicast_endpoint.protocol(), error);
        if (error.failed())
        {
            log.warning("failed to open socket on local address: {}, port: {}. Error: {}.", 
                unicast_endpoint.address().to_string(), unicast_endpoint.port(), error.message());
            return false;
        }
        
        m_rcv_socket->set_option(udp_t::socket::reuse_address(true), error);
        if (error.failed())
        {
            log.warning("failed to enable udp socket option: reuse_address. Error: {}.", error.message());
            return false;
        }
        
        m_rcv_socket->bind(unicast_endpoint, error);
        if (error.failed())
        {
            log.warning("failed to bind socket on local address: {}, port: {}. Error: {}.", 
                unicast_endpoint.address().to_string(), unicast_endpoint.port(), error.message());
            return false;
        }

        return !error.failed();
    }
} // ! namespace discnet::network