/*
 *
 */

#include <ranges>
#include <discnet_lib/route_manager.hpp>

namespace discnet
{
void route_manager_t::update(time_point_t current_time)
{
    for (routes_t& routes : m_adapter_routes | std::views::values)
    {
        for (route_t& route : routes)
        {
            if (is_route_online(route))
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

bool route_manager_t::add_route(const adapter_identifier_t& adapter_id, route_t& route)
{
    auto itr_adapter_routes = m_adapter_routes.find(adapter_id);
    if (itr_adapter_routes != m_adapter_routes.end())
    {
        
    }

    return false; // todo: implement
}
} // !namespace discnet
