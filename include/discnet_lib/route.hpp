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
        node_identifier_t m_node;
        address_v4_t m_adapter;
        address_v4_t m_reporter;
    };

    struct route_status_t
    {
        bool m_online;
        bool m_persistent;
        bool m_silent;
        uint8_t m_metric;
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
        route_identifier m_identifier;
        time_point_t m_last_tdp;
        time_point_t m_last_data_message;
        route_status_t m_status;
    };
    
    DISCNET_EXPORT bool is_route_online(const route_t& route);
    DISCNET_EXPORT bool is_direct_node(const route_identifier& route);
    DISCNET_EXPORT bool contains(const std::span<route_identifier>& routes, const route_identifier& route);
}