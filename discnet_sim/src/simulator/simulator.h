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
    class simulator
    {
    public:
        simulator();

        shared_discnet_node find_node(const node_identifier_t identifier);
        void add_node(const node_identifier_t identifier, const discnet::application::configuration_t& configuration, QTextEdit* log_handle);
        bool remove_node(const node_identifier_t identifier);

        bool add_router(const router_identifier_t& identifier, const router_properties& properties);
        void remove_router(const router_identifier_t& identifier);

        void add_adapter(const node_identifier_t node_identifier, const adapter_identifier_t adapter_identifier, const adapter_t adapter);
        void remove_adapter(const node_identifier_t node_identifier, const adapter_identifier_t adapter_identifier);

        void add_link(const node_identifier_t node_identifier, const adapter_identifier_t adapter_identifier, const router_identifier_t router_identifier);
        void remove_link(const node_identifier_t node_identifier, const adapter_identifier_t adapter_identifier);

        void set_traffic_manager_log_handle(QTextEdit* log_handle);
        void update(discnet::time_point_t current_time);
    private:
        std::mutex m_mutex;
        std::map<node_identifier_t, shared_discnet_node> m_nodes;
        shared_network_traffic_manager m_network_traffic_manager;
        
    };
} // !namespace discnet::sim::logic