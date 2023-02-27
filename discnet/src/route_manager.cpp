/*
 *
 */

#include <ranges>
#include <discnet/route_manager.hpp>
#include <boost/core/ignore_unused.hpp>

namespace discnet
{
void route_manager_t::update(const time_point_t& current_time)
{
    boost::ignore_unused(current_time);

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
    boost::ignore_unused(route);

    auto itr_adapter_routes = m_adapter_routes.find(adapter_id);
    if (itr_adapter_routes != m_adapter_routes.end())
    {
        
    }

    return false; // todo: implement
}
} // !namespace discnet
