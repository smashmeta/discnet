/*
 *
 */

#pragma once

#include <discnet_lib/typedefs.hpp>
#include <discnet_lib/node.hpp>

namespace discnet
{
    struct route_identifier
    {
        node_identifier_t m_node = init_required;
        address_v4_t m_adapter = init_required;
        address_v4_t m_reporter = init_required;
    };

    struct route_status_t
    {
        bool m_online = false;
        bool m_persistent = false;
        bool m_silent = false;
        uint8_t m_metric = 0;
    };

    /* 
        { 
            route_id: [ node: { 1010, 192.169.10.10 }, adapter: 192.169.10.1, reporter: 192.169.10.10 ] 
            last_tdp:  2022-12-09 14:23:11
            last_data: 1970-01-01 00:00:00 <no data received>
            status: [ online: true, persistent: false, silent: false ]
        }
    */
    struct route_t
    {
        route_identifier m_identifier = init_required;
        time_point_t m_last_tdp = time_point_t::min();
        time_point_t m_last_data_message = time_point_t::min();
        route_status_t m_status = route_status_t();
    };
    
    DISCNET_EXPORT bool is_route_online(const route_t& route);
    DISCNET_EXPORT bool is_direct_node(const route_identifier& route);
    DISCNET_EXPORT bool contains(const std::span<route_identifier>& routes, const route_identifier& route);
}