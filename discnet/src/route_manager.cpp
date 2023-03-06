/*
 *
 */

#include <ranges>
#include <boost/core/ignore_unused.hpp>
#include <whatlog/logger.hpp>
#include <discnet/route_manager.hpp>

namespace discnet
{
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
                    e_online_state_changed(route, false);
                }
            }
            else
            {
                if (route.m_status.m_online)
                {
                    route.m_status.m_online = false;
                    e_online_state_changed(route, true);
                }
            }
        }
    }
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
    node_identifier_t sender_identifier {.m_id = message.m_identifier, .m_address = network_info.m_sender};
    route_identifier route_id { .m_node = sender_identifier, .m_adapter = network_info.m_adapter, .m_reporter = network_info.m_sender };

    auto adapter_routes = m_adapter_routes.find(adapter->m_guid);
    if (adapter_routes == m_adapter_routes.end())
    {
        // new route detected
        route_status_t status {.m_online = true, .m_mtu = adapter->m_mtu };
        route_t route { .m_identifier = route_id, .m_last_discovery = network_info.m_reception_time, .m_status = status };
        m_adapter_routes.try_emplace(adapter->m_guid, routes_t{route});
    }
    else
    {
        auto [adapter_id, routes] = *adapter_routes;
        auto route = std::find_if(routes.begin(), routes.end(), [&](const auto& val) { return val.m_identifier == route_id; });
        if (route == routes.end())
        {
            log.warning("failed to find route.");
            return false;
        }

        route->m_last_discovery = network_info.m_reception_time;
    }
    
    return false; // todo: implement
}
} // !namespace discnet
