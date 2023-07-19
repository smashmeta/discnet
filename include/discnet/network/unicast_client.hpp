/*
 *
 */

#pragma once

#include <boost/asio.hpp>
#include <boost/core/ignore_unused.hpp>
#include <discnet/discnet.hpp>
#include <discnet/typedefs.hpp>
#include <discnet/network/data_handler.hpp>
#include <discnet/network/buffer.hpp>
#include <discnet/network/network_info.hpp>

namespace discnet::network
{
    struct unicast_info_t
    {
        discnet::address_t m_address;
        discnet::port_type_t m_port;
    };

    class data_handler;

    class unicast_client;
	using shared_unicast_client = std::shared_ptr<unicast_client>;

    class unicast_client : public std::enable_shared_from_this<unicast_client>
    {
    public:
        [[nodiscard]] static DISCNET_EXPORT shared_unicast_client create(discnet::shared_io_service io_service, unicast_info_t info, shared_data_handler data_handler);

        DISCNET_EXPORT bool open();
        DISCNET_EXPORT bool write(const discnet::network::buffer_t& buffer);
        DISCNET_EXPORT bool write(const discnet::address_t& recipient, const discnet::network::buffer_t& buffer);
        DISCNET_EXPORT void close();

        DISCNET_EXPORT unicast_info_t info() const;

    private:
        void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
        void handle_read(const boost::system::error_code& error, size_t bytes_received);

    protected:
        unicast_client(discnet::shared_io_service io_service, unicast_info_t info, shared_data_handler data_handler);

    private:
        bool open_unicast_snd_socket();
        bool open_unicast_rcv_socket();

        shared_data_handler m_data_handler;
        discnet::shared_io_service m_service;
        discnet::shared_udp_socket m_rcv_socket;
        discnet::shared_udp_socket m_snd_socket;
        std::vector<discnet::byte_t> m_rcv_buffer;
        boost::asio::ip::udp::endpoint m_rcv_endpoint;
        unicast_info_t m_info;
    };
} // !namespace discnet::network
