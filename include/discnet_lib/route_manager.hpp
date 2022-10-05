/*
 *
 */

#pragma once

#include <discnet_lib/route.hpp>

namespace discnet
{
    class route_manager
    {
    public:
        boost::signals2::signal<void(const route_t& curr, bool prev)> e_online_state_changed;

        void update(time_point_t current_time);

        std::map<route_identifier, route_t> m_routes;
        std::vector<route_identifier> m_active_routes;
    };
} // !namespace discnet