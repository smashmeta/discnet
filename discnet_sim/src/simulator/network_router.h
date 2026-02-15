/*
 *
 */

#pragma once

#include <format>
#include <discnet/typedefs.hpp>
#include "simulator/node.h"


namespace discnet::sim::logic
{
    using router_identifier_t = uint32_t;

    struct router_properties
    {
        discnet::duration_t m_latency;
        float m_drop_rate;
    };

    class network_router
    {
    public:
        network_router(const router_identifier_t& identifier, const router_properties& properties);
        
        void add_participant(const node_identifier_t node_identifier, network::shared_udp_client client);
        void remove_participant(const node_identifier_t node_identifier);

        void data_sent(const node_identifier_t node_identifier, const network::shared_udp_client& client, const network::udp_info_t& info, const discnet::network::buffer_t& buffer);
    private:
        router_identifier_t m_identifier;
        router_properties m_properties;
        std::map<node_identifier_t, network::shared_udp_client> m_participants;
    };
} // !namespace discnet::sim::logic