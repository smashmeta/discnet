/*
 *
 */

#include <discnet_lib/route_manager.hpp>

namespace discnet
{
void route_manager::update(time_point_t current_time)
{
    for (auto& route : m_routes)
    {
        if (is_route_online(route.second))
        {
            if (!route.second.m_online)
            {
                route.second.m_online = true;
                e_online_state_changed(route.second, false);
            }
        }
        else
        {
            if (route.second.m_online)
            {
                route.second.m_online = false;
                e_online_state_changed(route.second, true);
            }
        }
    }
}
} // !namespace discnet
