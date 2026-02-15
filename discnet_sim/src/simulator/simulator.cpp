/*
 *
 */

#include <QTextEdit>
#include "simulator/simulator.h"


namespace discnet::sim::logic
{
    simulator::simulator()
        : m_network_traffic_manager(std::make_shared<network_traffic_manager>())
    {
        // nothing for now
    }

    shared_discnet_node simulator::find_node(const node_identifier_t identifier)
    {
        auto itr_node = m_nodes.find(identifier);
        if (itr_node != m_nodes.end())
        {
            return itr_node->second;
        }

        return {};
    }

    void simulator::add_node(const node_identifier_t identifier, const discnet::application::configuration_t& configuration, QTextEdit* log_handle)
    {
        auto node = std::make_shared<discnet_node>(identifier, configuration, m_network_traffic_manager, log_handle);
        node->initialize();
        
        std::lock_guard<std::mutex> lock {m_mutex};
        m_nodes.insert(std::make_pair(identifier, node));
    }

    bool simulator::remove_node(const node_identifier_t identifier)
    {
        std::lock_guard<std::mutex> lock {m_mutex};

        auto node = m_nodes.find(identifier);
        if (node != m_nodes.end())
        {
            
            m_network_traffic_manager->remove_node(identifier);
            m_nodes.erase(node);
            return true;
        }

        return false;
    }

    bool simulator::add_router(const router_identifier_t& identifier, const router_properties& properties)
    {
        return m_network_traffic_manager->add_router(identifier, properties);
    }

    void simulator::remove_router(const router_identifier_t& identifier)
    {
        m_network_traffic_manager->remove_router(identifier);
    }

    void simulator::add_adapter(const node_identifier_t node_identifier, const adapter_identifier_t adapter_identifier, const adapter_t adapter)
    {
        auto node = m_nodes.find(node_identifier);
        if (node != m_nodes.end())
        {
            node->second->add_adapter(adapter_identifier, adapter);
        }
    }

    void simulator::remove_adapter(const node_identifier_t node_identifier, const adapter_identifier_t adapter_identifier)
    {
        auto node = m_nodes.find(node_identifier);
        if (node != m_nodes.end())
        {
            m_network_traffic_manager->remove_link(node_identifier, adapter_identifier);
            node->second->remove_adapter(adapter_identifier);
        }
    }

    void simulator::add_link(const node_identifier_t node_identifier, const adapter_identifier_t adapter_identifier, const router_identifier_t router_identifier)
    {
        m_network_traffic_manager->add_link(node_identifier, adapter_identifier, router_identifier);
    }

    void simulator::remove_link(const node_identifier_t node_identifier, const adapter_identifier_t adapter_identifier)
    {
        m_network_traffic_manager->remove_link(node_identifier, adapter_identifier);
    }

    void simulator::set_traffic_manager_log_handle(QTextEdit* log_handle)
    {
        m_network_traffic_manager->set_log_handle(log_handle);
    }

    void simulator::update(discnet::time_point_t current_time)
    {
        std::lock_guard<std::mutex> lock {m_mutex};
        for (auto& [id, node] : m_nodes)
        {
            node->update(current_time);
        }
    }
} // !namespace discnet::sim::logic