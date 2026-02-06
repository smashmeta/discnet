/*
 *
 */

#include <spdlog/spdlog.h>
#include <discnet/network/udp_client.hpp>


namespace discnet::network
{
    iudp_client::iudp_client(udp_info_t info, const data_received_func& func)
        : m_info(info), m_data_received_func(func)
    {
        // nothing for now
    }

    shared_udp_client udp_client::create(const discnet::application::configuration_t& configuration, discnet::shared_io_context io_context, udp_info_t info, const data_received_func& callback_func) 
    {
        return std::shared_ptr<udp_client>(new udp_client(configuration, io_context, info, callback_func));
    }

    udp_client::udp_client(const discnet::application::configuration_t& configuration, discnet::shared_io_context io_context, udp_info_t info, const data_received_func& callback_func)
        :   iudp_client(info, callback_func),
            m_configuration(configuration),
            m_logger(spdlog::get(configuration.m_log_instance_id)),
            m_context(io_context), 
            m_socket(new discnet::socket_t{*io_context.get()}),
            m_rcv_buffer(12560, '\0')
    {
        // nothing for now
    }

    bool udp_client::open()
    {
        bool result = internal_open();
        if (result)
        {
            boost::asio::mutable_buffer data_buffer((void*)m_rcv_buffer.data(), m_rcv_buffer.size());
            m_socket->async_receive_from(data_buffer, m_rcv_endpoint, 
                boost::bind(&udp_client::handle_read, shared_from_this(), 
                    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        }
        else
        {
            close();
        }

        return result;
    }

    bool udp_client::internal_open()
    {
        using udp_t = boost::asio::ip::udp;
        namespace multicast = boost::asio::ip::multicast;

        m_logger->info("setting up udp socket - addr: {}, port: {}, multicast: {}.", 
           m_info.m_adapter.to_string(),
           m_info.m_port,
           m_info.m_multicast.to_string());

        discnet::error_code_t error;
        
        m_socket->open(boost::asio::ip::udp::v4(), error);
        if (error)
        {
            m_logger->warn("failed to open ipv4 socket.");
            return false;
        }

        m_socket->set_option(udp_t::socket::reuse_address(true), error);
        if (error)
        {
            return false;
        }

#ifdef _WIN32
        m_socket->bind(udp_t::endpoint(m_info.m_adapter, m_info.m_port), error);
        if (error)
        {
            m_logger->warn("failed to bind socket to adapter {}. Error: {}.", m_info.m_adapter.to_string(), error.message());
            return false;
        }

#elif
        m_socket->bind(udp_t::endpoint(udp_t::v4(), m_info.m_port), error);
        if (error)
        {
            m_logger->warn("failed to set bind socket to ip-v4. Error: {}.", error.message());
            return false;
        }


        // todo: implement on linux
        /*
        #include <ifaddrs.h>
        #include <netinet/in.h>
        #include <arpa/inet.h>
        #include <string>

        std::string get_interface_name_from_ip(const std::string& ip_to_find) {
            struct ifaddrs *addrs, *tmp;
            std::string interface_name = "";

            if (getifaddrs(&addrs) == -1) return "";

            tmp = addrs;
            while (tmp) {
                if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET) {
                    struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
                    std::string ip = inet_ntoa(pAddr->sin_addr);
                    if (ip == ip_to_find) {
                        interface_name = tmp->ifa_name;
                        break;
                    }
                }
                tmp = tmp->ifa_next;
            }

            freeifaddrs(addrs);
            return interface_name;
        }
        */

        int rc = setsockopt(m_socket->native_handle(), SOL_SOCKET, SO_BINDTODEVICE, interface_name.c_str(), interface_name.size());
        if (rc < 0) 
        {
            m_logger->warn("failed to set bind socket to native device interface {}. ErrorCode: {}.", interface_name, rc);
            return false;
        }
#endif

        m_socket->set_option(multicast::join_group(m_info.m_multicast, m_info.m_adapter));
        if (error)
        {
            m_logger->warn("failed to set join multicast group ({}:{}). Error: {}.", m_info.m_multicast.to_string(), m_info.m_port, error.message());
            return false;
        }

        m_socket->set_option(multicast::outbound_interface(m_info.m_adapter), error);
        if (error)
        {
            m_logger->warn("failed to set socket option (outbound_interface: {}). Error: {}.", m_info.m_adapter.to_string(), error.message());
            return false;
        }

        m_socket->set_option(multicast::enable_loopback(false), error);
        if (error)
        {
            m_logger->warn("failed to set socket option (enable_loopback: false). Error: {}.", error.message());
            return false;
        }

        return true;
    }

    bool udp_client::write(const discnet::network::buffer_t& buffer)
    {
        using udp_t = boost::asio::ip::udp;
        
        auto const_buffer = boost::asio::const_buffer(buffer.data().data(), buffer.data().size());
        udp_t::endpoint multicast_endpoint{m_info.m_multicast, m_info.m_port};
        m_socket->async_send_to(const_buffer, multicast_endpoint, 
            boost::bind(&udp_client::handle_write, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        
        return true;
    }

    bool udp_client::write(const discnet::network::buffer_t& buffer, const discnet::address_t& recipient)
    {
        using udp_t = boost::asio::ip::udp;
        
        auto const_buffer = boost::asio::const_buffer(buffer.data().data(), buffer.data().size());
        udp_t::endpoint unicast_endpoint{recipient, m_info.m_port};
        m_socket->async_send_to(const_buffer, unicast_endpoint, 
            boost::bind(&udp_client::handle_write, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        
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
        if (!error)
        {
            m_data_received_func(
                boost::asio::const_buffer(m_rcv_buffer.data(), bytes_received), 
                m_rcv_endpoint.address().to_v4(), m_info.m_multicast
            );
        }
        else
        {
            if (error == boost::asio::error::connection_aborted)
            {
                m_logger->info("closing multicast connection on adapter {}.", m_info.m_adapter.to_string());
                close();
            }
            else
            {
                m_logger->warn("error reading data. id: {}, message: {}.", error.value(), error.message());
            }
            
            return;
        }

        boost::asio::mutable_buffer data_buffer((void*)m_rcv_buffer.data(), m_rcv_buffer.size());
            m_socket->async_receive_from(data_buffer, m_rcv_endpoint, 
                boost::bind(&udp_client::handle_read, this, 
                    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void udp_client::close()
    {
        m_socket->close();
    }
} // !namespace discnet::network