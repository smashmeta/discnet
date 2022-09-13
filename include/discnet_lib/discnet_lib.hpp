/*
 *
 */

#pragma once

#include <string>
#include <span>
#include <cstddef>
#include <chrono>
#include <boost/asio.hpp>

#ifdef DISCNET_DLL
#  define DISCNET_EXPORT __declspec(dllexport)
#else
#  define DISCNET_EXPORT __declspec(dllimport)
#endif

namespace discnet
{
    typedef boost::asio::ip::address_v4 address_v4_t;
    typedef std::chrono::system_clock::time_point time_point_t;

    struct node_identifier
    {
        uint16_t m_id;
        address_v4_t m_address;
    };

    struct route_identifier
    {
        node_identifier m_node;
        address_v4_t m_adapter;
        address_v4_t m_reporter;
    };

    /* 
        { 
            route_id: [ node: { 1010, 192.169.10.10 }, adapter: 192.169.10.1, reporter: 192.169.10.10 ] 
            last_tdp:  2022-12-09 14:23:11
            last_data: 1970-01-01 00:00:00 <no data received>
        }
    */
    struct route_t
    {
        route_identifier m_identifier;
        time_point_t m_last_tdp;
        time_point_t m_last_data_message;
        bool m_persistent;
    };

    DISCNET_EXPORT bool is_route_online(const route_t& route);

    DISCNET_EXPORT bool is_direct_node(const route_identifier& route);

    DISCNET_EXPORT bool is_unique_route(const std::span<route_identifier>& routes, const route_identifier& route);

    DISCNET_EXPORT std::string bytes_to_hex_string(const std::span<std::byte>& buffer);
} // !namesapce discnet