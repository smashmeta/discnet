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
        bool connected();
        void close();
    private:
        discnet::shared_io_service m_io_service;
        discnet::shared_udp_socket m_udp_socket;
    };
} // !namespace discnet::network