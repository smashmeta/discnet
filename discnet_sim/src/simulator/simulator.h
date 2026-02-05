/*
 *
 */

#pragma once

#include <discnet/application/configuration.hpp>
#include "simulator/network_traffic_manager.h"
#include "simulator/node.h"

class QTextEdit;

namespace discnet::sim::logic
{
    using instance_identifier = uint16_t;

    class simulator
    {
    public:
        simulator()
            : m_network_traffic_manager(std::make_shared<network_traffic_manager>())
        {
            // nothing for now
        }

        shared_discnet_node find_node(const uint16_t node_id);
        instance_identifier add_instance(const discnet::application::configuration_t& configuration, QTextEdit* log_handle);
        bool add_switch(switch_identifier id);
        bool remove_instance(const instance_identifier& id);
        void set_traffic_manager_log_handle(QTextEdit* log_handle);
        void update(discnet::time_point_t current_time);
    private:
        std::mutex m_mutex;
        std::map<instance_identifier, shared_discnet_node> m_nodes;
        shared_network_traffic_manager m_network_traffic_manager;
        
    };
} // !namespace discnet::sim::logic