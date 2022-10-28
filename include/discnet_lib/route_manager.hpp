/*
 *
 */

#pragma once

#include <discnet_lib/adapter.hpp>
#include <discnet_lib/route.hpp>

namespace discnet
{
    class route_manager_t
    {
        typedef std::vector<route_t> routes_t;
        typedef std::map<adapter_identifier_t, routes_t> adapter_routes_map_t;
    public:
        boost::signals2::signal<void(const route_t& curr, bool prev)> e_online_state_changed;

        void update(const time_point_t& current_time);
        bool add_route(const adapter_identifier_t& adapter_id, route_t& route);

        adapter_routes_map_t m_adapter_routes;
    };
} // !namespace discnet