/*
 *
 */

#pragma once

#include <QTextEdit>
#include <spdlog/sinks/qt_sinks.h>
#include <discnet/typedefs.hpp>
#include <discnet/network/udp_client.hpp>
#include "simulator/network_router.h"
#include "simulator/network_adapter_fetcher.h"
#include "simulator/udp_client.h"


namespace discnet::sim::logic
{
    struct adapter_entry
    {
        adapter_identifier_t m_adapter_identifier;
        std::optional<router_identifier_t> m_router;
        network::shared_udp_client m_client;
    };

    class network_traffic_manager
    {
    public:
        network_traffic_manager();

        void set_log_handle(QTextEdit* log_handle);

        void data_sent(const node_identifier_t identifier, const network::udp_info_t& info, const discnet::network::buffer_t& buffer);
        void data_sent(const node_identifier_t identifier, const network::udp_info_t& info, const discnet::network::buffer_t& buffer, const discnet::address_t& recipient);
        
        void add_node(const node_identifier_t node_identifier, const adapter_identifier_t adapter_identifier, network::shared_udp_client client);
        void remove_node(const node_identifier_t node_identifier);

        bool add_router(const router_identifier_t& identifier, const router_properties& properties);
        void remove_router(const router_identifier_t& identifier);

        bool add_link(const node_identifier_t node_identifier, const adapter_identifier_t adapter_identifier, const router_identifier_t router_identifier);
        void remove_link(const node_identifier_t node_identifier, const adapter_identifier_t adapter_identifier);

    private:
        std::shared_ptr<spdlog::logger> m_network_logger;
        std::map<node_identifier_t, std::vector<adapter_entry>> m_nodes;
        std::map<router_identifier_t, network_router> m_routers;
    };

    using shared_network_traffic_manager = std::shared_ptr<network_traffic_manager>;
} // !namespace discnet::sim::logic