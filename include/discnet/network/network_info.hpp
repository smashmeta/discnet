/*
 *
 */

#pragma once

#include <discnet/discnet.hpp>

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
        time_point_t m_reception_time = time_point_t::time_point::min();
        address_t m_sender;
        address_t m_receiver;
        address_t m_adapter;
    };
} // !namespace discnet::network