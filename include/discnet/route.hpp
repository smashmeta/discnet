/*
 *
 */

#pragma once

#include <discnet/typedefs.hpp>
#include <discnet/node.hpp>
#include <discnet/adapter.hpp>

namespace discnet
{
    struct route_identifier_t
    {
        bool operator==(const route_identifier_t&) const = default;
        node_identifier_t m_node;
        address_t m_adapter;
        address_t m_reporter;
    };

    struct route_status_t
    {
        bool m_online = false;
        bool m_persistent = false;
        bool m_silent = false;
        jumps_t m_jumps = jumps_t{};
        mtu_type_t m_mtu = 0;
    };

    /* 
        { 
            route_id: [ node: { 1010, 192.169.10.10 }, adapter: 192.169.10.1, reporter: 192.169.10.10 ] 
            last_discovery:  2022-12-09 14:23:11
            last_data: 1970-01-01 00:00:00 <no data received>
            status: [ online: true, persistent: false, silent: false, jumps: {256, 512}, mtu: 1500 ]
        }
    */
    struct route_t
    {
        route_identifier_t m_identifier;
        time_point_t m_last_discovery = time_point_t::min();
        time_point_t m_last_data_message = time_point_t::min();
        route_status_t m_status = route_status_t();
    };

    /*
        {
            route_id: [node: {1010, 192.169.10.10}, adapter: 192.169.10.11, reporter: 192.169.10.10]
            gateway: 192.169.10.1
            metric: 256
            enabled: true
        }
    */
    struct persistent_route_t
    {
        route_identifier_t m_identifier = init_required;
        discnet::address_t m_gateway;
        discnet::metric_t m_metric;
        bool m_enabled;
    };
    
    DISCNET_EXPORT bool is_shorter_route(const route_t& lhs, const route_t& rhs);
    DISCNET_EXPORT bool is_route_online(const route_t& route, const time_point_t& current_time);
    DISCNET_EXPORT bool is_direct_node(const route_identifier_t& route);
    DISCNET_EXPORT bool contains(const std::span<route_identifier_t>& routes, const route_identifier_t& route);

    DISCNET_EXPORT std::string to_string(const route_identifier_t& route);
}