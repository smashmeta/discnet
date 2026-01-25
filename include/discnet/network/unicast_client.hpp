/*
 *
 */

#pragma once

#include <boost/asio.hpp>
#include <boost/core/ignore_unused.hpp>
#include <discnet/discnet.hpp>
#include <discnet/application/configuration.hpp>
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

    class iunicast_client;
	using shared_unicast_client = std::shared_ptr<iunicast_client>;

    class iunicast_client
    {
    public:
        DISCNET_EXPORT iunicast_client(unicast_info_t info, const data_received_func& func);

        virtual bool open() = 0;
        virtual bool write(const discnet::address_t& recipient, const discnet::network::buffer_t& buffer) = 0;
        virtual void close() = 0;
    protected:
        unicast_info_t m_info;
        data_received_func m_data_received_func;
    };
    
    class unicast_client : public iunicast_client, public std::enable_shared_from_this<unicast_client>
    {
    public:
        [[nodiscard]] static DISCNET_EXPORT shared_unicast_client create(const discnet::application::shared_loggers& loggers, discnet::shared_io_context io_context, unicast_info_t info, const data_received_func& callback_func);

        DISCNET_EXPORT bool open();
        // DISCNET_EXPORT bool write(const discnet::network::buffer_t& buffer);
        DISCNET_EXPORT bool write(const discnet::address_t& recipient, const discnet::network::buffer_t& buffer);
        DISCNET_EXPORT void close();

        DISCNET_EXPORT unicast_info_t info() const;

    private:
        void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
        void handle_read(const boost::system::error_code& error, size_t bytes_received);

    protected:
        unicast_client(const discnet::application::shared_loggers& loggers, discnet::shared_io_context io_context, unicast_info_t info, const data_received_func& callback_func);

    private:
        bool open_unicast_snd_socket();
        bool open_unicast_rcv_socket();

        discnet::application::shared_loggers m_loggers;
        discnet::shared_io_context m_context;
        discnet::shared_udp_socket m_rcv_socket;
        discnet::shared_udp_socket m_snd_socket;
        std::vector<discnet::byte_t> m_rcv_buffer;
        boost::asio::ip::udp::endpoint m_rcv_endpoint;
    };
} // !namespace discnet::network
