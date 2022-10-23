/*
 *
 */

#pragma once

#include <boost/asio.hpp>
#include <discnet_lib/discnet_lib.hpp>
#include <discnet_lib/typedefs.hpp>

namespace discnet::network
{
    class udp_client
    {
    public:
        udp_client(discnet::shared_io_service io_service)
            : m_service(io_service), m_socket(new discnet::socket_t{*io_service.get()}), m_open(false)
        {
            // nothing for now
        }

        bool open(discnet::address_v4_t remote_address, discnet::port_type_t remote_port)
        {
            using udp_t = boost::asio::ip::udp;
            discnet::error_code_t error;
            udp_t::endpoint remote_endpoint{remote_address, remote_port};
            m_socket->open(udp_t::v4(), error);
            m_open = !error.failed();

            return error.failed() ? false : true;
        }

        void close()
        {
            m_socket->close();
            m_open = false;
        }
    private:
        discnet::shared_io_service m_service;
        discnet::shared_udp_socket m_socket;
        bool m_open;
    };
} // !namespace discnet::network