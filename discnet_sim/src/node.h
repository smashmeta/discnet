/*
 *
 */

#pragma once

#include <memory>
#include <discnet/typedefs.hpp>
#include <discnet/application/configuration.hpp>
#include <QTextEdit>

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

    struct asio_context_t;
    using shared_asio_context_t = std::shared_ptr<asio_context_t>;

    class discovery_message_handler;
    using shared_discovery_message_handler = std::shared_ptr<discovery_message_handler>;

    class transmission_handler;
    using shared_transmission_handler = std::shared_ptr<transmission_handler>;

    class discnet_node
    {
    public:
        discnet_node(const application::configuration_t& configuration, QTextEdit* text_edit);
        
        bool initialize();
        void update(time_point_t current_time);
    private:
        application::configuration_t m_configuration;
        shared_asio_context_t m_asio_context;
        discnet::shared_adapter_manager m_adapter_manager;
        discnet::shared_route_manager m_route_manager;
        discnet::network::shared_network_handler m_network_handler;
        shared_discovery_message_handler m_discovery_message_handler;
        shared_transmission_handler m_transmission_handler;
        discnet::application::shared_loggers m_loggers;
    };

    using shared_discnet_node = std::shared_ptr<discnet_node>;
} // !namespace discnet
