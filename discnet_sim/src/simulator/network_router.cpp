/*
 *
 */

#include <simulator/network_router.h>
#include "simulator/udp_client.h"


namespace discnet::sim::logic
{
    network_router::network_router(const router_identifier_t& identifier, const router_properties& properties)
        : m_identifier(identifier), m_properties(properties)
    {
        // nothing for now
    }

    void network_router::add_participant(const node_identifier_t node_identifier, network::shared_udp_client client)
    {
        m_participants.insert({node_identifier, client});
    }

    void network_router::remove_participant(const node_identifier_t node_identifier)
    {
        m_participants.erase(node_identifier);
    }

    void network_router::data_sent([[maybe_unused]] const node_identifier_t node_identifier, [[maybe_unused]] const network::shared_udp_client& client, 
        [[maybe_unused]] const network::udp_info_t& info, [[maybe_unused]] const discnet::network::buffer_t& buffer)
    {
        for (auto& participant : m_participants)
        {
            if (participant.first != node_identifier)
            {
                auto sim_adapter = std::dynamic_pointer_cast<simulator_udp_client>(participant.second);
                if (sim_adapter)
                {
                    sim_adapter->receive_bytes(buffer, info.m_adapter);
                }
            }
        }
    }
} // !namespace dicsnet::sim::logic