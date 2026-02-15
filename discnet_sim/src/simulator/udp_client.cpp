/*
 *
 */

#include "simulator/udp_client.h"
#include "simulator/network_traffic_manager.h"
#include "simulator/network_adapter_fetcher.h"

namespace discnet::sim::logic
{
    simulator_udp_client::simulator_udp_client(const node_identifier_t identifier, const discnet::application::configuration_t& configuration, const shared_network_traffic_manager& ntm, const network::udp_info_t& info, const network::data_received_func& func)
        : m_identifier(identifier), network::iudp_client(info, func), m_configuration(configuration), m_network_traffic_manager(ntm)
    {
        // nothing for now
    }

    bool simulator_udp_client::open() 
    { 
        return true; 
    }

    bool simulator_udp_client::write(const discnet::network::buffer_t& buffer) 
    { 
        m_network_traffic_manager->data_sent(m_identifier, m_info, buffer);
        return true; 
    }

    bool simulator_udp_client::write(const discnet::network::buffer_t& buffer, const discnet::address_t& recipient) 
    { 
        m_network_traffic_manager->data_sent(m_identifier, m_info, buffer, recipient);
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

    simulator_client_creator::simulator_client_creator(const node_identifier_t identifier, const discnet::application::configuration_t& configuration, 
        const shared_network_traffic_manager& ntm, const shared_simulator_adapter_fetcher& adapter_fetcher)
        : m_identifier(identifier), m_configuration(configuration), m_logger(spdlog::get(configuration.m_log_instance_id)), 
            m_network_traffic_manager(ntm), m_adapter_fetcher(adapter_fetcher)
    {
        // nothing for now
    }

    discnet::network::shared_udp_client simulator_client_creator::create(const discnet::network::udp_info_t& info, const discnet::network::data_received_func& callback_func) 
    { 
        adapter_identifier_t adapter_identifier = m_adapter_fetcher->get_identifier(info.m_adapter);
        auto result = std::make_shared<simulator_udp_client>(m_identifier, m_configuration, m_network_traffic_manager, info, callback_func);
        m_network_traffic_manager->add_node(m_identifier, adapter_identifier, result);
        return result; 
    }
} // !namespace discnet::sim::logic