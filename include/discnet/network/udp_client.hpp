/*
 *
 */

#pragma once

#include <boost/asio.hpp>
#include <boost/core/ignore_unused.hpp>
#include <discnet/discnet.hpp>
#include <discnet/typedefs.hpp>
#include <discnet/network/buffer.hpp>
#include <discnet/network/network_info.hpp>

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
        boost::signals2::signal<void(buffer_t&, const network_info_t&)> e_received_data;
    public:
        DISCNET_EXPORT udp_client(discnet::shared_io_service io_service, multicast_info info, size_t buffer_size);

        DISCNET_EXPORT bool open();
        DISCNET_EXPORT bool write(const discnet::network::buffer_t& buffer);
        DISCNET_EXPORT void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
        DISCNET_EXPORT void handle_read(const boost::system::error_code& error, size_t bytes_received);
        DISCNET_EXPORT void close();
    private:
        bool open_multicast_snd_socket();
        bool open_multicast_rcv_socket();

        discnet::shared_io_service m_service;
        discnet::shared_udp_socket m_rcv_socket;
        discnet::shared_udp_socket m_snd_socket;
        std::vector<discnet::byte_t> m_rcv_buffer;
        boost::asio::ip::udp::endpoint m_rcv_endpoint;
        multicast_info m_info;
    };
} // !namespace discnet::network