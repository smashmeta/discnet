/*
 *
 */

#pragma once

#include <discnet_lib/discnet_lib.hpp>

namespace discnet::network
{
    /*
    info [
        time: now(),
        sender: 192.200.1.10
        receiver: 238.200.200.200
        adapter: 192.200.1.2
    ]
    */
    struct network_info_t
    {
        time_point_t m_reception_time;
        address_v4_t m_sender;
        address_v4_t m_receiver;
        address_v4_t m_adapter;
    };
} // !namespace discnet::network