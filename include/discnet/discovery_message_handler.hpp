/*
 *
 */

#pragma once

#include <discnet/typedefs.hpp>
#include <discnet/route_manager.hpp>
#include <discnet/network/network_handler.hpp>

namespace discnet
{
    class discovery_message_handler
    {
    public:
        DISCNET_EXPORT discovery_message_handler(discnet::network::shared_network_handler network_handler, shared_route_manager route_manager);
        DISCNET_EXPORT void handle_discovery_message(const network::messages::discovery_message_t& message, const network::network_info_t& network_info);
    private:
        shared_route_manager m_route_manager;
    };
} // !namespace discnet