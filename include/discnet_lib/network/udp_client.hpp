/*
 *
 */

#pragma once

#include <boost/asio.hpp>
#include <discnet_lib/discnet_lib.hpp>
#include <discnet_lib/typedefs.hpp>
#include <discnet_lib/network/buffer.hpp>

namespace discnet::network
{
    struct multicast_info
    {
        discnet::address_v4_t m_adapter_address = init_required;
        discnet::address_v4_t m_multicast_address = init_required;
        discnet::port_type_t m_multicast_port = init_required;
    };

    class udp_client
    {
    public:
        udp_client(discnet::shared_io_service io_service, multicast_info info)
            :   m_service(io_service), 
                m_rcv_socket(new discnet::socket_t{*io_service.get()}),
                m_snd_socket(new discnet::socket_t{*io_service.get()}),
                m_info(info)
        {
            // nothing for now
        }

        bool open()
        {
            return open_multicast_rcv_socket() && open_multicast_snd_socket();
        }

        bool send(const discnet::network::buffer_t& buffer)
        {
            using udp_t = boost::asio::ip::udp;
            
            discnet::error_code_t error;
            auto const_buffer = boost::asio::const_buffer(buffer.data().data(), buffer.data().size());
            udp_t::endpoint multicast_endpoint{m_info.m_multicast_address, m_info.m_multicast_port};
            m_snd_socket->async_send_to(const_buffer, multicast_endpoint, 
                boost::bind(&udp_client::handle_send, this, boost::asio::placeholders::error));
        }

        void handle_send(const discnet::error_code_t error)
        {
            
        }

        void close()
        {
            m_rcv_socket->close();
            m_snd_socket->close();
        }
    private:
        bool open_multicast_snd_socket()
        {
            using udp_t = boost::asio::ip::udp;
            namespace multicast = boost::asio::ip::multicast;

            discnet::error_code_t error;
            udp_t::endpoint multicast_endpoint{m_info.m_multicast_address, m_info.m_multicast_port};
            
            m_snd_socket->open(multicast_endpoint.protocol(), error);
            m_snd_socket->set_option(multicast::outbound_interface(m_info.m_adapter_address), error);

            return !error.failed();
        }

        bool open_multicast_rcv_socket()
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

        discnet::shared_io_service m_service;
        discnet::shared_udp_socket m_rcv_socket;
        discnet::shared_udp_socket m_snd_socket;
        multicast_info m_info;
    };
} // !namespace discnet::network