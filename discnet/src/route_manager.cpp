/*
 *
 */

#include <ranges>
#include <boost/core/ignore_unused.hpp>
#include <whatlog/logger.hpp>
#include <discnet/route_manager.hpp>

namespace discnet
{
route_manager::route_manager(shared_adapter_manager adapter_manager)
    : m_adapter_manager(adapter_manager)
{
    // nothing for now
}

void route_manager::update(const time_point_t& current_time)
{
    whatlog::logger log("route_manager::update");

    for (routes_t& routes : m_adapter_routes | std::views::values)
    {
        for (route_t& route : routes)
        {
            if (is_route_online(route, current_time))
            {
                if (!route.m_status.m_online)
                {
                    route.m_status.m_online = true;
                    log.info("route [id: {}, adapter: {}, reporter: {}] now online.", 
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
                    log.info("route [id: {}, adapter: {}, reporter: {}] has gone offline.", 
                        route.m_identifier.m_node.m_id, route.m_identifier.m_adapter.to_string(), 
                        route.m_identifier.m_reporter.to_string());
                    
                    e_online_state_changed(route, true);
                }
            }
        }
    }
}

bool route_manager::process_node(const adapter_t& adapter, const network_info_t& network_info, const route_identifier& route_id)
{
    whatlog::logger log("route_manager::process_node");
    auto adapter_routes = m_adapter_routes.find(adapter.m_guid);
    if (adapter_routes == m_adapter_routes.end())
    {
        // new route detected
        route_status_t status {.m_online = true, .m_mtu = adapter.m_mtu};
        route_t route {.m_identifier = route_id, .m_last_discovery = network_info.m_reception_time, .m_status = status};
        route.m_status.m_jumps.push_back(256);

        m_adapter_routes.try_emplace(adapter.m_guid, routes_t{route});

        std::string route_info_str = std::format("(id: {}, address: {} - jumps: {})", route.m_identifier.m_node.m_id, 
            route.m_identifier.m_node.m_address.to_string(), discnet::to_string(route.m_status.m_jumps));
        log.info("new route {} detected.", route_info_str);
    }
    else
    {
        auto& [adapter_id, routes] = *adapter_routes;
        auto route = std::find_if(routes.begin(), routes.end(), [&](const auto& val) { return val.m_identifier == route_id; });
        if (route == routes.end())
        {
            log.warning("failed to find route.");
            return false;
        }

        route->m_last_discovery = network_info.m_reception_time;
    }

    return true;
}

bool route_manager::process(const network_info_t& network_info, const discovery_message_t& message)
{
    whatlog::logger log("route_manager::process");
    log.info("received discovery message from {} on adapter {}.", message.m_identifier, network_info.m_adapter.to_string());

    auto adapter = m_adapter_manager->find_adapter(network_info.m_adapter);
    if (!adapter)
    {
        log.warning("failed to find adapter {} in adapter manager.", network_info.m_adapter.to_string());
        return false;
    }

    // update sender information
    node_identifier_t sender_id {.m_id = message.m_identifier, .m_address = network_info.m_sender};
    route_identifier route_id {.m_node = sender_id, .m_adapter = network_info.m_adapter, .m_reporter = network_info.m_sender};
    bool result = process_node(adapter.value(), network_info, route_id);
    
    for (const auto& indirect_node : message.m_nodes)
    {
        node_identifier_t indirect_node_id {.m_id = indirect_node.m_identifier, .m_address = indirect_node.m_address};
        route_identifier indirect_route_id {.m_node = indirect_node_id, .m_adapter = network_info.m_adapter, .m_reporter = network_info.m_sender};
        result &= process_node(adapter.value(), network_info, indirect_route_id);
    }
    
    return result;
}

route_manager::routes_t route_manager::find_routes(const adapter_identifier_t& outbound_adapter)
{
    routes_t result;

    for (auto [adapter, routes] : m_adapter_routes)
    {
        if (adapter != outbound_adapter)
        {
            for (const route_t& route : routes)
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

    return result;
}
} // !namespace discnet
