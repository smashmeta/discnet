/*
 *
 */

#include <ranges>
#include <spdlog/spdlog.h>
#include <boost/core/ignore_unused.hpp>
#include <discnet/route_manager.hpp>

namespace discnet
{
route_manager::route_manager(const discnet::application::shared_loggers& loggers, shared_adapter_manager adapter_manager, network::shared_network_handler network_handler)
    : m_loggers(loggers), m_adapter_manager(adapter_manager), m_network_handler(network_handler)
{
    network_handler->e_discovery_message_received.connect(
                std::bind(&route_manager::process_discovery_message, this, std::placeholders::_1, std::placeholders::_2));
}

void route_manager::update(const time_point_t& current_time)
{
    for (routes_t& routes : m_adapter_routes | std::views::values)
    {
        for (route_t& route : routes)
        {
            if (is_route_online(route, current_time))
            {
                if (!route.m_status.m_online)
                {
                    route.m_status.m_online = true;
                    m_loggers->m_logger->info("route [id: {}, adapter: {}, reporter: {}] now online.", 
                        route.m_identifier.m_node.m_id, route.m_identifier.m_adapter.to_string(), 
                        route.m_identifier.m_reporter.to_string());
                    
                    e_online_state_changed(route, false);
                }
            }
            else
            {
                if (route.m_status.m_online)
                {
                    route.m_status.m_online = false;
                    m_loggers->m_logger->info("route [id: {}, adapter: {}, reporter: {}] has gone offline.", 
                        route.m_identifier.m_node.m_id, route.m_identifier.m_adapter.to_string(), 
                        route.m_identifier.m_reporter.to_string());
                    
                    e_online_state_changed(route, true);
                }
            }
        }
    }
}

bool route_manager::process_discovery_message(const discovery_message_t& message, const network_info_t& network_info)
{
    m_loggers->m_logger->info("received discovery message from {} on adapter {}.", message.m_identifier, network_info.m_adapter.to_string());

    auto adapter = m_adapter_manager->find_adapter(network_info.m_adapter);
    if (!adapter)  
    {
        m_loggers->m_logger->warn("failed to find adapter {} in adapter manager.", network_info.m_adapter.to_string());
        return false;
    }

    // update sender information
    jumps_t jumps {256};
    node_identifier_t sender_id {.m_id = message.m_identifier, .m_address = network_info.m_sender};
    route_identifier_t route_id {.m_node = sender_id, .m_adapter = network_info.m_adapter, .m_reporter = network_info.m_sender};
    bool result = process_route(adapter.value(), network_info.m_reception_time, route_id, jumps);
    
    for (const auto& indirect_node : message.m_nodes)
    {
        node_identifier_t indirect_node_id {.m_id = indirect_node.m_identifier, .m_address = indirect_node.m_address};
        route_identifier_t indirect_route_id {.m_node = indirect_node_id, .m_adapter = network_info.m_adapter, .m_reporter = network_info.m_sender};
        jumps_t indirect_jumps = jumps;
        
        std::ranges::copy(indirect_node.m_jumps, std::back_inserter(indirect_jumps));
        result &= process_route(adapter.value(), network_info.m_reception_time, indirect_route_id, indirect_jumps);
    }
    
    return result;
}

bool route_manager::process_persistent_route(const persistent_route_t& route, const discnet::time_point_t& time)
{
    m_loggers->m_logger->info("received persistent_route message on adapter {}.", route.m_identifier.m_adapter.to_string());

    auto adapter = m_adapter_manager->find_adapter(route.m_identifier.m_adapter);
    if (!adapter)
    {
        m_loggers->m_logger->warn("failed to find adapter {} in adapter manager.", route.m_identifier.m_adapter.to_string());
        return false;
    }

    node_identifier_t node_id = route.m_identifier.m_node;
    route_identifier_t route_id {.m_node = node_id, .m_adapter = route.m_identifier.m_adapter, .m_reporter = route.m_identifier.m_reporter};
    
    bool result = process_route(adapter.value(), time, route_id, {route.m_metric});
    if (result)
    {
        auto itr_routes = m_adapter_routes.find(adapter->m_guid);
        if (itr_routes == m_adapter_routes.end())
        {
            m_loggers->m_logger->warn("failed to find adapter {} in adapter manager.", route.m_identifier.m_adapter.to_string());
            return false;
        }

        auto& [guid, routes] = *itr_routes;
        auto itr_route = std::find_if(routes.begin(), routes.end(), [&](const auto& val){return val.m_identifier == route_id;});
        if (itr_route == routes.end())
        {
            m_loggers->m_logger->warn("failed to find insertered route {} in route_manager.", discnet::to_string(route_id));
        }

        itr_route->m_status.m_persistent = route.m_enabled;
    }

    return result;
}

bool route_manager::process_route(const adapter_t& adapter, const discnet::time_point_t& time, const route_identifier_t& route_id, const jumps_t& jumps)
{
    auto itr_adapter_routes = m_adapter_routes.find(adapter.m_guid);
    if (itr_adapter_routes == m_adapter_routes.end())
    {
        // new route detected
        route_status_t status {.m_online = true, .m_mtu = adapter.m_mtu};
        route_t route {.m_identifier = route_id, .m_last_discovery = time, .m_status = status};
        route.m_status.m_jumps = jumps;
        m_adapter_routes.try_emplace(adapter.m_guid, routes_t{route});

        std::string route_info_str = std::format("(id: {}, address: {} - jumps: {})", route.m_identifier.m_node.m_id, 
            route.m_identifier.m_node.m_address.to_string(), discnet::to_string(route.m_status.m_jumps));
        m_loggers->m_logger->info("new route {} detected.", route_info_str);

        e_new_route(route);
    }
    else
    {
        auto& [adapter_id, routes] = *itr_adapter_routes;
        auto itr_route = std::find_if(routes.begin(), routes.end(), [&](const auto& val) { return val.m_identifier == route_id; });
        if (itr_route == routes.end())
        {
            // new route detected
            route_status_t status {.m_online = true, .m_mtu = adapter.m_mtu};
            route_t route {.m_identifier = route_id, .m_last_discovery = time, .m_status = status};
            route.m_status.m_jumps = jumps;
            routes.push_back(route);
            
            std::string route_info_str = std::format("(id: {}, address: {} - jumps: {})", route.m_identifier.m_node.m_id, 
            route.m_identifier.m_node.m_address.to_string(), discnet::to_string(route.m_status.m_jumps));
            m_loggers->m_logger->info("new route {} detected.", route_info_str);

            e_new_route(route);
        }
        else
        {
            // updating existing route
            itr_route->m_last_discovery = time;
        }
    }

    return true;
}

route_manager::routes_t route_manager::find_routes_for_adapter(const adapter_identifier_t& outbound_adapter)
{
    routes_t result;

    for (auto [adapter, routes] : m_adapter_routes)
    {
        if (adapter != outbound_adapter)
        {
            for (const route_t& route : routes)
            {
                if (route.m_status.m_online)
                {
                    auto existing_route = std::find_if(result.begin(), result.end(), 
                        [&](const auto& val) { return val.m_identifier == route.m_identifier; });
                    if (existing_route != result.end())
                    {
                        if (is_shorter_route(route, *existing_route))
                        {
                            // replace with shorter route
                            result.erase(existing_route);
                            result.push_back(route);
                        }
                    }
                    else
                    {
                        result.push_back(route);
                    }
                }
            }
        }
    }

    return result;
}

route_manager::routes_t route_manager::find_routes_on_adapter(const adapter_identifier_t& outbound_adapter)
{
    routes_t result;

    for (auto [adapter, routes] : m_adapter_routes)
    {
        if (adapter == outbound_adapter)
        {
            result.insert(result.end(), routes.begin(), routes.end());
            break; 
        }
    }

    return result;
}
} // !namespace discnet
