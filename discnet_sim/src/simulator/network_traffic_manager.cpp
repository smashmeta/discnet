/*
 *
 */

#include "simulator/network_traffic_manager.h"
#include "simulator/udp_client.h"

namespace discnet::sim::logic
{
    network_traffic_manager::network_traffic_manager()
        : m_network_logger(spdlog::get("traffic"))
    {
        // nothing for now
    }

    void network_traffic_manager::set_log_handle(QTextEdit* log_handle)
    {
        m_network_logger = spdlog::qt_logger_mt("sim_network_log", log_handle);
    }

    void network_traffic_manager::data_sent(const node_identifier_t identifier, const network::udp_info_t& info, const discnet::network::buffer_t& buffer)
    {
        auto node = m_nodes.find(identifier);
        if (node == m_nodes.end())
        {
            m_network_logger->error("Failed to find node entry for node identifier {}. Message dropped.", identifier);
            return;
        }

        for (auto& adapter : node->second)
        {
            if (adapter.m_client->info().m_adapter == info.m_adapter)
            {
                if (adapter.m_router)
                {
                    auto router = m_routers.find(adapter.m_router.value());
                    if (router != m_routers.end())
                    {
                        router->second.data_sent(identifier, adapter.m_client, info, buffer);
                    }
                    else
                    {
                        // failed to find the router that the adapter is linked to
                    }
                }
                else
                {
                    // this adapter is currently not linked to a router. dropping bytes
                }
            }
        }
    }

    void network_traffic_manager::data_sent([[maybe_unused]] const node_identifier_t identifier, [[maybe_unused]] const network::udp_info_t& info, [[maybe_unused]] const discnet::network::buffer_t& buffer, [[maybe_unused]] const discnet::address_t& recipient)
    {
    // todo: implement
    }

    void network_traffic_manager::add_node(const node_identifier_t node_identifier, const adapter_identifier_t adapter_identifier, network::shared_udp_client client)
    {
        adapter_entry entry {.m_adapter_identifier = adapter_identifier, .m_router = {}, .m_client = client};
        auto node = m_nodes.find(node_identifier);
        if (node != m_nodes.end())
        {
            
            node->second.push_back(entry);
        }
        else
        {
            m_nodes.insert({node_identifier, {entry}});
        }
    }

    void network_traffic_manager::remove_node(const node_identifier_t node_identifier)
    {
        auto node = m_nodes.find(node_identifier);
        if (node != m_nodes.end())
        {
            for (auto& entry : node->second)
            {
                if (entry.m_router)
                {
                    auto router = m_routers.find(entry.m_router.value());
                    if (router != m_routers.end())
                    {
                        router->second.remove_participant(node_identifier);
                    }
                }
            }

            m_nodes.erase(node_identifier);
        }
    }

    bool network_traffic_manager::add_router(const router_identifier_t& identifier, const router_properties& properties)
    {
        auto existing = m_routers.find(identifier);
        if (existing == m_routers.end())
        {
            m_routers.insert({identifier, network_router(identifier, properties)});
        }

        return false;
    }

     void network_traffic_manager::remove_router(const router_identifier_t& identifier)
    {
        m_routers.erase(identifier);
    }

    bool network_traffic_manager::add_link(const node_identifier_t node_identifier, const adapter_identifier_t adapter_identifier, const router_identifier_t router_identifier)
    {
        auto node = m_nodes.find(node_identifier);
        auto router = m_routers.find(router_identifier);
        if (node != m_nodes.end() && router != m_routers.end())
        {
            for (auto& entry : node->second)
            {
                if (entry.m_adapter_identifier == adapter_identifier)
                {
                    entry.m_router = router_identifier;
                    router->second.add_participant(node_identifier, entry.m_client);
                    return true;
                }
            }
        }

        return false;
    }

    void network_traffic_manager::remove_link(const node_identifier_t node_identifier, const adapter_identifier_t adapter_identifier)
    {
        auto node = m_nodes.find(node_identifier);
        if (node != m_nodes.end())
        {
            for (auto& entry : node->second)
            {
                if (entry.m_adapter_identifier == adapter_identifier)
                {
                    if (entry.m_router)
                    {
                        auto router = m_routers.find(entry.m_router.value());
                        router->second.remove_participant(node_identifier);
                        entry.m_router.reset();
                    }

                    break;
                }
            }
        }
        
    }
} // !namespace discnet::sim::logic