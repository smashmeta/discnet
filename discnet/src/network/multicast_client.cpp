/*
 *
 */

#include <fmt/format.h>
#include <whatlog/logger.hpp>
#include <discnet/network/data_handler.hpp>
#include <discnet/network/multicast_client.hpp>


namespace discnet::network
{
    multicast_client::multicast_client(discnet::shared_io_service io_service, multicast_info info, size_t buffer_size)
        :   m_data_handler(new data_handler(4095)),
            m_service(io_service), 
            m_rcv_socket(new discnet::socket_t{*io_service.get()}),
            m_snd_socket(new discnet::socket_t{*io_service.get()}),
            m_rcv_buffer(buffer_size, '\0'),
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

        auto streams = m_data_handler->process();
        for (const data_stream_packets_t& stream : streams)
        {
            for (const messages::packet_t& packet : stream.m_packets)
            {
                info.m_sender = stream.m_identifier.m_sender_ip;
                info.m_receiver = stream.m_identifier.m_recipient_ip;

                for (const auto& message : packet.m_messages)
                {
                    if (std::holds_alternative<messages::discovery_message_t>(message))
                    {
                        auto discovery_message = std::get<messages::discovery_message_t>(message);
                        e_discovery_message_received(discovery_message, info);
                    }
                    else if (std::holds_alternative<messages::data_message_t>(message))
                    {
                        auto data_message = std::get<messages::data_message_t>(message);
                        e_data_message_received(data_message, info);
                    }
                }
            }
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
        whatlog::logger log("open_multicast_snd_socket");

        discnet::error_code_t error;
        udp_t::endpoint multicast_endpoint{m_info.m_multicast_address, m_info.m_multicast_port};
        
        m_snd_socket->open(multicast_endpoint.protocol(), error);
        if (error.failed())
        {
            log.warning(fmt::format("failed to open socket on local address: {}, port: {}. Error: {}", 
                multicast_endpoint.address().to_string(), multicast_endpoint.port(), error.message()));
            return false;
        }

        m_snd_socket->set_option(multicast::outbound_interface(m_info.m_adapter_address), error);
        if (error.failed())
        {
            log.warning(fmt::format("failed to enable udp socket option: outbound_interface with address: {}. Error: {}", 
                m_info.m_adapter_address.to_string(), error.message()));
        }

        return !error.failed();
    }

    bool multicast_client::open_multicast_rcv_socket()
    {
        using udp_t = boost::asio::ip::udp;
        namespace multicast = boost::asio::ip::multicast;
        whatlog::logger log("open_multicast_rcv_socket");

        log.info(fmt::format("setting up mc listening port: addr: {}, port: {}, on adapter {}.", 
            m_info.m_multicast_address.to_string(),
            m_info.m_multicast_port,
            m_info.m_adapter_address.to_string()));

        udp_t::endpoint multicast_endpoint{discnet::address_t::any(), m_info.m_multicast_port};
        
        discnet::error_code_t error;
        m_rcv_socket->open(multicast_endpoint.protocol(), error);
        if (error.failed())
        {
            log.warning(fmt::format("failed to open socket on local address: {}, port: {}. Error: {}", 
                multicast_endpoint.address().to_string(), multicast_endpoint.port(), error.message()));
            return false;
        }
        
        m_rcv_socket->set_option(udp_t::socket::reuse_address(true), error);
        if (error.failed())
        {
            log.warning(fmt::format("failed to enable udp socket option: reuse_address. Error: {}", error.message()));
            return false;
        }

        m_rcv_socket->set_option(multicast::enable_loopback(false), error);
        if (error.failed())
        {
            log.warning(fmt::format("failed to enable udp socket option: disable_loopback. Error: {}", error.message()));
            return false;
        }
        
        m_rcv_socket->bind(multicast_endpoint, error);
        if (error.failed())
        {
            log.warning(fmt::format("failed to bind socket on local address: {}, port: {}. Error: {}", 
                multicast_endpoint.address().to_string(), multicast_endpoint.port(), error.message()));
            return false;
        }

        m_rcv_socket->set_option(multicast::join_group(m_info.m_multicast_address, m_info.m_adapter_address), error);
        if (error.failed())
        {
            log.warning(fmt::format("failed to enable udp socket option: join_group. multicast-address: {}, adapter-address: {}. Error: {}", 
                m_info.m_multicast_address.to_string(), m_info.m_adapter_address.to_string(), error.message()));
            return false;
        }

        return !error.failed();
    }
} // !namespace discnet::network