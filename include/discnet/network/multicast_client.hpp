/*
 *
 */

#pragma once

#include <functional>
#include <boost/asio.hpp>
#include <boost/core/ignore_unused.hpp>
#include <discnet/discnet.hpp>
#include <discnet/typedefs.hpp>
#include <discnet/application/configuration.hpp>
#include <discnet/network/data_handler.hpp>
#include <discnet/network/buffer.hpp>
#include <discnet/network/network_info.hpp>

namespace discnet::network
{
    struct multicast_info_t
    {
        discnet::address_t m_adapter_address;
        discnet::address_t m_multicast_address;
        discnet::port_type_t m_multicast_port;
    };

    class data_handler;

    class imulticast_client;
	using shared_multicast_client = std::shared_ptr<imulticast_client>;

    class imulticast_client
    {
    public:
        DISCNET_EXPORT imulticast_client(multicast_info_t info, const data_received_func& func);
    
        virtual bool open() = 0;
        virtual bool write(const discnet::network::buffer_t& buffer) = 0;
        virtual void close() = 0;
        virtual multicast_info_t info() const = 0;
    protected:
        multicast_info_t m_info;    
        data_received_func m_data_received_func;
    };

    class simulator_multicast_client : public imulticast_client
    {
    public:
        simulator_multicast_client(multicast_info_t info, const data_received_func& func)
            : imulticast_client(info, func)
        {
            // nothing for now
        }
    
        bool open() override { return false; }
        bool write([[maybe_unused]] const discnet::network::buffer_t& buffer) override { return false; }
        void close() override { }
        multicast_info_t info() const override { return m_info; }
    };

    class multicast_client : public imulticast_client, public std::enable_shared_from_this<multicast_client>
    {
    public:
        static DISCNET_EXPORT shared_multicast_client create(const discnet::application::shared_loggers& loggers, discnet::shared_io_context io_context, multicast_info_t info, const data_received_func& callback_func);

        DISCNET_EXPORT bool open();
        DISCNET_EXPORT bool write(const discnet::network::buffer_t& buffer);
        DISCNET_EXPORT void close();

        DISCNET_EXPORT multicast_info_t info() const override;

    private:
        void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
        void handle_read(const boost::system::error_code& error, size_t bytes_received);

    protected:
        multicast_client(const discnet::application::shared_loggers& loggers, discnet::shared_io_context io_context, multicast_info_t info, const data_received_func& callback_func);

    private:
        bool open_multicast_snd_socket();
        bool open_multicast_rcv_socket();

        discnet::application::shared_loggers m_loggers;
        discnet::shared_io_context m_context;
        discnet::shared_udp_socket m_rcv_socket;
        discnet::shared_udp_socket m_snd_socket;
        std::vector<discnet::byte_t> m_rcv_buffer;
        boost::asio::ip::udp::endpoint m_rcv_endpoint;
    };
} // !namespace discnet::network