/*
 *
 */

#pragma once

#include <memory>
#include <discnet/discnet.hpp>
#include <discnet/route.hpp>
#include <discnet/adapter.hpp>
#include <discnet/adapter_manager.hpp>
#include <discnet/network/messages/discovery_message.hpp>
#include <discnet/network/network_info.hpp>

namespace discnet
{
    class route_manager
    {
        typedef std::vector<route_t> routes_t;
        typedef std::map<adapter_identifier_t, routes_t> adapter_routes_map_t;
        typedef discnet::network::messages::discovery_message_t discovery_message_t;
        typedef discnet::network::network_info_t network_info_t;
    public:
        boost::signals2::signal<void(const route_t& curr, bool prev)> e_online_state_changed;

        DISCNET_EXPORT void update(const time_point_t& current_time);
        DISCNET_EXPORT bool process(const network_info_t& adapter_info, const discovery_message_t& route);
    
    private:
        shared_adapter_manager m_adapter_manager;
        adapter_routes_map_t m_adapter_routes;
    };

    typedef std::shared_ptr<route_manager> shared_route_manager;
} // !namespace discnet