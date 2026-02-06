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
    class data_handler;

    struct udp_info_t
    {
        discnet::address_t m_adapter;
        discnet::port_type_t m_port;
        discnet::address_t m_multicast;
    };

    class iudp_client;
	using shared_udp_client = std::shared_ptr<iudp_client>;

    class iudp_client
    {
    public:
        DISCNET_EXPORT iudp_client(udp_info_t info, const data_received_func& func);

        virtual bool open() = 0;
        virtual bool write(const discnet::network::buffer_t& buffer) = 0;
        virtual bool write(const discnet::network::buffer_t& buffer, const discnet::address_t& recipient) = 0;
        virtual void close() = 0;
        
        udp_info_t info() const { return m_info; }

    protected:
        udp_info_t m_info;
        data_received_func m_data_received_func;
    };
    
    class udp_client : public iudp_client, public std::enable_shared_from_this<udp_client>
    {
    public:
        [[nodiscard]] static DISCNET_EXPORT shared_udp_client create(const discnet::application::configuration_t& configuration, discnet::shared_io_context io_context, udp_info_t info, const data_received_func& callback_func);

        DISCNET_EXPORT bool open() override;
        DISCNET_EXPORT bool write(const discnet::network::buffer_t& buffer) override;
        DISCNET_EXPORT bool write(const discnet::network::buffer_t& buffer, const discnet::address_t& recipient) override;
        DISCNET_EXPORT void close() override;

    private:
        void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
        void handle_read(const boost::system::error_code& error, size_t bytes_received);

    protected:
        udp_client(const discnet::application::configuration_t& configuration, discnet::shared_io_context io_context, udp_info_t info, const data_received_func& callback_func);

    private:
        bool internal_open();

        discnet::application::configuration_t m_configuration;
        discnet::shared_logger m_logger;
        discnet::shared_io_context m_context;
        discnet::shared_udp_socket m_socket;
        std::vector<discnet::byte_t> m_rcv_buffer;
        discnet::endpoint_t m_rcv_endpoint;
    };
} // !namespace discnet::network
