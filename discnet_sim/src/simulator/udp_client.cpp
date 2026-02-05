/*
 *
 */

#include "simulator/udp_client.h"
#include "simulator/network_traffic_manager.h"

namespace discnet::sim::logic
{
    simulator_udp_client::simulator_udp_client(const uint16_t node_id, const shared_network_traffic_manager& ntm, const network::udp_info_t& info, const network::data_received_func& func)
        : network::iudp_client(info, func), m_node_id(node_id), m_network_traffic_manager(ntm)
    {
        // nothing for now
    }

    bool simulator_udp_client::open() 
    { 
        return true; 
    }

    bool simulator_udp_client::write(const discnet::network::buffer_t& buffer) 
    { 
        m_network_traffic_manager->data_sent(m_node_id, m_info, buffer);
        return true; 
    }

    bool simulator_udp_client::write(const discnet::network::buffer_t& buffer, const discnet::address_t& recipient) 
    { 
        m_network_traffic_manager->data_sent(m_node_id, m_info, buffer, recipient);
        return true; 
    }

    void simulator_udp_client::close() 
    { 
        // todo: implement
    }

    void simulator_udp_client::receive_bytes(const discnet::network::buffer_t& buffer, const discnet::address_t& sender)
    {
        m_data_received_func(buffer.const_buffer(), sender, m_info.m_multicast);
    }

    simulator_client_creator::simulator_client_creator(const discnet::application::shared_loggers& loggers, const uint16_t node_id, const shared_network_traffic_manager& ntm)
        : m_loggers(loggers), m_node_id(node_id), m_network_traffic_manager(ntm)
    {
        // nothing for now
    }

    discnet::network::shared_udp_client simulator_client_creator::create(const discnet::network::udp_info_t& info, const discnet::network::data_received_func& callback_func) 
    { 
        auto result = std::make_shared<simulator_udp_client>(m_node_id, m_network_traffic_manager, info, callback_func);
        m_network_traffic_manager->register_client(m_node_id, result);
        return result; 
    }
} // !namespace discnet::sim::logic