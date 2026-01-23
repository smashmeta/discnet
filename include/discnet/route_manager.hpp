/*
 *
 */

#pragma once

#include <memory>
#include <discnet/discnet.hpp>
#include <discnet/route.hpp>
#include <discnet/adapter.hpp>
#include <discnet/adapter_manager.hpp>
#include <discnet/application/configuration.hpp>
#include <discnet/network/network_handler.hpp>
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
        boost::signals2::signal<void(const route_t& route)> e_new_route;
        boost::signals2::signal<void(const route_t& curr, bool prev)> e_online_state_changed;

        DISCNET_EXPORT route_manager(const discnet::application::shared_loggers& loggers, shared_adapter_manager adapter_manager, network::shared_network_handler network_handler);

        DISCNET_EXPORT void update(const time_point_t& current_time);
        DISCNET_EXPORT bool process_discovery_message(const discovery_message_t& route, const network_info_t& adapter_info);
        DISCNET_EXPORT bool process_persistent_route(const persistent_route_t& route, const discnet::time_point_t& time);
        
        DISCNET_EXPORT routes_t find_routes_for_adapter(const adapter_identifier_t& outbound_adapter);
        DISCNET_EXPORT routes_t find_routes_on_adapter(const adapter_identifier_t& adapter);
    private:
        bool process_route(const adapter_t& adapter, const discnet::time_point_t& network_info, const route_identifier_t& identifier, const jumps_t& jumps);

        discnet::application::shared_loggers m_loggers;
        shared_adapter_manager m_adapter_manager;
        network::shared_network_handler m_network_handler;
        adapter_routes_map_t m_adapter_routes;
    };

    typedef std::shared_ptr<route_manager> shared_route_manager;
} // !namespace discnet