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
        discnet::address_t m_adapter_address;
        discnet::address_t m_multicast_address;
        discnet::port_type_t m_multicast_port;
    };

    class data_handler;

    class multicast_client;
	using shared_multicast_client = std::shared_ptr<multicast_client>;

    class multicast_client : public std::enable_shared_from_this<multicast_client>
    {
    public:
        boost::signals2::signal<void(const messages::discovery_message_t&, const network_info_t&)> e_discovery_message_received;
        boost::signals2::signal<void(const messages::data_message_t&, const network_info_t&)> e_data_message_received;
    public:
        [[nodiscard]] static DISCNET_EXPORT shared_multicast_client create(discnet::shared_io_service io_service, multicast_info info, size_t buffer_size);

        DISCNET_EXPORT void process();
        DISCNET_EXPORT bool open();
        DISCNET_EXPORT bool write(const discnet::network::buffer_t& buffer);
        DISCNET_EXPORT void close();

    private:
        DISCNET_EXPORT void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
        DISCNET_EXPORT void handle_read(const boost::system::error_code& error, size_t bytes_received);

    protected:
        multicast_client(discnet::shared_io_service io_service, multicast_info info, size_t buffer_size);

    private:
        bool open_multicast_snd_socket();
        bool open_multicast_rcv_socket();

        data_handler* m_data_handler;
        discnet::shared_io_service m_service;
        discnet::shared_udp_socket m_rcv_socket;
        discnet::shared_udp_socket m_snd_socket;
        std::vector<discnet::byte_t> m_rcv_buffer;
        boost::asio::ip::udp::endpoint m_rcv_endpoint;
        multicast_info m_info;
    };
} // !namespace discnet::network