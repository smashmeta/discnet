/*
 *
 */

#pragma once

#include <QTextEdit>
#include <spdlog/sinks/qt_sinks.h>
#include <discnet/typedefs.hpp>
#include <discnet/network/udp_client.hpp>
#include "simulator/network_switch.h"
#include "simulator/network_adapter_fetcher.h"
#include "simulator/udp_client.h"


namespace discnet::sim::logic
{
    class network_traffic_manager
    {
    public:
        void data_sent(const uint16_t node_id, const network::udp_info_t& info, const discnet::network::buffer_t& buffer);
        void data_sent(const uint16_t node_id, const network::udp_info_t& info, const discnet::network::buffer_t& buffer, const discnet::address_t& recipient);
        void register_client(const uint16_t node_id, network::shared_udp_client client);
        void set_log_handle(QTextEdit* log_handle);
        bool add_switch(const network_switch& val);
    private:
        std::shared_ptr<spdlog::logger> m_network_logger;
        std::map<uint16_t, std::vector<network::shared_udp_client>> m_clients;
        std::map<std::string, network_switch> m_switches;
    };

    using shared_network_traffic_manager = std::shared_ptr<network_traffic_manager>;
} // !namespace discnet::sim::logic