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

bool route_manager::process(const network_info_t& adapter_info, const discovery_message_t& message)
{
    whatlog::logger log("route_manager::process");
    log.info("received discovery message from {} on adapter {}.", message.m_identifier, adapter_info.m_adapter.to_string());
    return false; // todo: implement
}
} // !namespace discnet
