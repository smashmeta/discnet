/*
 *
 */

#pragma once

#include <memory>
#include <discnet/typedefs.hpp>
#include <discnet/application/configuration.hpp>
#include <discnet/adapter_manager.hpp>
#include <discnet/network/network_handler.hpp>
#include <QTextEdit>
#include <spdlog/sinks/qt_sinks.h>

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

    class discovery_message_handler;
    using shared_discovery_message_handler = std::shared_ptr<discovery_message_handler>;

    class transmission_handler;
    using shared_transmission_handler = std::shared_ptr<transmission_handler>;
}

namespace discnet::sim::logic
{
    class simulator_adapter_fetcher;
    using shared_simulator_adapter_fetcher = std::shared_ptr<simulator_adapter_fetcher>;

    class network_traffic_manager;
    using shared_network_traffic_manager = std::shared_ptr<network_traffic_manager>;

    class discnet_node
    {
    public:
        discnet_node(const application::configuration_t& configuration, const shared_network_traffic_manager& ntm, QTextEdit* text_edit);
        ~discnet_node();

        void add_adapter(const adapter_t& adapter);
        
        bool initialize();
        void update(time_point_t current_time);
    private:
        application::configuration_t m_configuration;
        shared_simulator_adapter_fetcher m_adapter_fetcher;
        discnet::shared_adapter_manager m_adapter_manager;
        discnet::shared_route_manager m_route_manager;
        discnet::network::shared_network_handler m_network_handler;
        shared_discovery_message_handler m_discovery_message_handler;
        shared_transmission_handler m_transmission_handler;
        discnet::application::shared_loggers m_loggers;
        shared_network_traffic_manager m_network_traffic_manager;
    };

    using shared_discnet_node = std::shared_ptr<discnet_node>;
} // !namespace discnet::sim::logic
