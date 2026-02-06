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

    shared_discnet_node simulator::find_node(const uint16_t node_id)
    {
        auto itr_node = m_nodes.find(node_id);
        if (itr_node != m_nodes.end())
        {
            return itr_node->second;
        }

        return {};
    }

    instance_identifier simulator::add_instance(const discnet::application::configuration_t& configuration, QTextEdit* log_handle)
    {
        auto node = std::make_shared<discnet_node>(configuration, m_network_traffic_manager, log_handle);
        node->initialize();
        
        std::lock_guard<std::mutex> lock {m_mutex};
        static uint32_t sequence_number = 1;
        instance_identifier node_id = configuration.m_node_id;
        m_nodes.insert(std::make_pair(node_id, node));
        return node_id;
    }

    bool simulator::add_switch(switch_identifier id)
    {
        return m_network_traffic_manager->add_switch(network_switch(id));
    }

    bool simulator::remove_instance(const instance_identifier& id)
    {
        std::lock_guard<std::mutex> lock {m_mutex};
        auto instance_itr = m_nodes.find(id);
        if (instance_itr != m_nodes.end())
        {
            m_nodes.erase(instance_itr);
            return true;
        }

        return false;
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