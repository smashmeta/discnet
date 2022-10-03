/*
 *
 */

#pragma once

#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <discnet_lib/discnet_lib.hpp>

namespace discnet
{
    typedef boost::asio::ip::address_v4 address_v4_t;
    typedef std::chrono::system_clock::time_point time_point_t;
    typedef boost::uuids::uuid uuid_t;
    typedef std::pair<address_v4_t, address_v4_t> address_mask_v4_t; 
}