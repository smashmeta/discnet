/*
 *
 */

#include "simulator/network_traffic_manager.h"
#include "simulator/udp_client.h"

namespace discnet::sim::logic
{
    void network_traffic_manager::data_sent(const uint16_t node_id, const network::udp_info_t& info, const discnet::network::buffer_t& buffer)
    {
        if (m_network_logger)
        {
            std::string log_entry = std::format("node: {}, adapter: {}, multicast: [addr: {}, port: {}] - {}",
                node_id, info.m_adapter.to_string(), info.m_adapter.to_string(), info.m_port, discnet::bytes_to_hex_string(buffer.data()));
            m_network_logger->info(log_entry);

            std::string subnet_mask_sender;
            {
                auto ip_str = info.m_adapter.to_string();
                auto last_segment_pos = ip_str.find_last_of('.');
                subnet_mask_sender = ip_str.substr(0, last_segment_pos);
            }

            for (auto& [id, adapters] : m_clients)
            {
                if (id != node_id)
                {
                    for (auto& adapter : adapters)
                    {
                        auto sim_adapter = std::dynamic_pointer_cast<simulator_udp_client>(adapter);
                        if (sim_adapter)
                        {
                            std::string subnet_mask_receiver;
                            {
                                auto ip_str = adapter->info().m_adapter.to_string();
                                auto last_segment_pos = ip_str.find_last_of('.');
                                subnet_mask_receiver = ip_str.substr(0, last_segment_pos);
                            }

                            if (subnet_mask_sender == subnet_mask_receiver)
                            {
                                sim_adapter->receive_bytes(buffer, info.m_adapter);
                            }
                        }
                    }
                }
            }
        }
    }

    void network_traffic_manager::data_sent([[maybe_unused]] const uint16_t node_id, [[maybe_unused]] const network::udp_info_t& info, [[maybe_unused]] const discnet::network::buffer_t& buffer, [[maybe_unused]] const discnet::address_t& recipient)
    {
    // todo: implement
    }

    void network_traffic_manager::register_client(const uint16_t node_id, network::shared_udp_client client)
    {
        auto itr_client = m_clients.find(node_id);
        if (itr_client != m_clients.end())
        {
            itr_client->second.push_back(client);
        }
        else
        {
            m_clients.insert({node_id, {client}});
        }
    }

    void network_traffic_manager::set_log_handle(QTextEdit* log_handle)
    {
        m_network_logger = spdlog::qt_logger_mt("sim_network_log", log_handle);
    }

    bool network_traffic_manager::add_switch(const network_switch& val)
    {
        auto existing = m_switches.find(val.get_subnet());
        if (existing == m_switches.end())
        {
            m_switches.insert({val.get_subnet(), val});
        }

        return false;
    }
} // !namespace discnet::sim::logic