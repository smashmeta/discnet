/*
 *
 */

#pragma once

#include <memory>
#include <discnet/typedefs.hpp>
#include <discnet/application/configuration.hpp>

namespace discnet
{
    class adapter_manager;
    using shared_adapter_manager = std::shared_ptr<adapter_manager>;

    class route_manager;
    using shared_route_manager = std::shared_ptr<route_manager>;

namespace network
{
    class network_handler;
    using shared_network_handler = std::shared_ptr<network_handler>;
} // !namespace network

namespace main
{
    struct asio_context_t;
    using shared_asio_context_t = std::shared_ptr<asio_context_t>;

    class discovery_message_handler;
    using shared_discovery_message_handler = std::shared_ptr<discovery_message_handler>;

    class transmission_handler;
    using shared_transmission_handler = std::shared_ptr<transmission_handler>;

    class application
    {
    public:
        application(discnet::application::configuration_t configuration);
        
        bool initialize();
        void update(discnet::time_point_t current_time);
    private:
        discnet::application::configuration_t m_configuration;
        shared_asio_context_t m_asio_context;
        discnet::shared_adapter_manager m_adapter_manager;
        discnet::shared_route_manager m_route_manager;
        discnet::network::shared_network_handler m_network_handler;
        shared_discovery_message_handler m_discovery_message_handler;
        shared_transmission_handler m_transmission_handler;
    };
} // !namespace main
} // !namespace discnet
