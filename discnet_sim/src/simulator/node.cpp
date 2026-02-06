/*
 *
 */

#include <spdlog/spdlog.h>
#include <spdlog/sinks/qt_sinks.h>
#include <discnet/application/configuration.hpp>
#include <discnet/adapter_manager.hpp>
#include <discnet/route_manager.hpp>
#include <discnet/network/network_handler.hpp>
#include <discnet/transmission_handler.hpp>
#include <discnet/discovery_message_handler.hpp>
#include "simulator/network_traffic_manager.h"
#include "node.h"

namespace discnet::sim::logic
{
    static int instance_id = 0;
    discnet_node::discnet_node(const application::configuration_t& configuration, const shared_network_traffic_manager& ntm, QTextEdit* text_edit)
        : m_configuration(configuration), m_network_traffic_manager(ntm)
    {
        // nothing for now
        m_logger = spdlog::qt_logger_mt(configuration.m_log_instance_id, text_edit);
    }

    discnet_node::~discnet_node()
    {
        m_logger->info("shutting down discnet node [{}].", m_configuration.m_node_id);
    }

    void discnet_node::add_adapter(const adapter_t& adapter)
    {
        m_adapter_fetcher->add_adapter(adapter);
    }

    bool discnet_node::initialize()
    {
        m_logger->info(std::format("discnet - node id: {}, multicast: [addr: {}, port: {}].", 
            m_configuration.m_node_id, m_configuration.m_multicast_address.to_string(), m_configuration.m_multicast_port));

        m_logger->info("setting up adapter_fetcher...");
        m_adapter_fetcher = std::make_shared<simulator_adapter_fetcher>();
        m_logger->info("setting up adapter_manager...");
        m_adapter_manager = std::make_shared<adapter_manager>(m_configuration, m_adapter_fetcher);

        m_logger->info("setting up network_handler...");
        m_network_handler = std::make_shared<network::network_handler>(m_configuration, m_adapter_manager, std::make_shared<simulator_client_creator>(m_configuration, m_network_traffic_manager));
        m_logger->info("setting up route_manager...");
        m_route_manager = std::make_shared<route_manager>(m_configuration, m_adapter_manager, m_network_handler);
        m_logger->info("setting up transmission_handler...");
        m_transmission_handler = std::make_shared<transmission_handler>(m_configuration, m_route_manager, m_network_handler, m_adapter_manager);

        return true;
    }

    void discnet_node::update(discnet::time_point_t current_time)
    {
        m_adapter_manager->update();
        m_network_handler->update(current_time);
        m_route_manager->update(current_time);
        m_transmission_handler->update(current_time);
    }
} // !namespace discnet::main