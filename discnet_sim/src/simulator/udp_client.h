/*
 *
 */

#pragma once

#include <discnet/network/udp_client.hpp>
#include <discnet/network/network_handler.hpp>
#include "simulator/node.h"


namespace discnet::sim::logic
{
    class network_traffic_manager;
    using shared_network_traffic_manager = std::shared_ptr<network_traffic_manager>;

    class simulator_adapter_fetcher;
    using shared_simulator_adapter_fetcher = std::shared_ptr<simulator_adapter_fetcher>;

    class simulator_udp_client : public network::iudp_client
    {
    public:
        simulator_udp_client(const node_identifier_t identifier, const discnet::application::configuration_t& configuration, const shared_network_traffic_manager& ntm, const network::udp_info_t& info, 
            const network::data_received_func& func);
    
        bool open() override;
        bool write(const discnet::network::buffer_t& buffer) override;
        bool write(const discnet::network::buffer_t& buffer, const discnet::address_t& recipient) override;
        void close() override;
        
        void receive_bytes(const discnet::network::buffer_t& buffer, const discnet::address_t& sender);

    private:
        node_identifier_t m_identifier;
        discnet::application::configuration_t m_configuration;
        shared_network_traffic_manager m_network_traffic_manager;
    };

    struct simulator_client_creator : public discnet::network::iclient_creator
    {
        simulator_client_creator(const node_identifier_t identifier, const discnet::application::configuration_t& configuration, 
            const shared_network_traffic_manager& ntm, const shared_simulator_adapter_fetcher& adapter_fetcher);
        discnet::network::shared_udp_client create(const discnet::network::udp_info_t& info, const discnet::network::data_received_func& callback_func) override;

    private:
        node_identifier_t m_identifier;
        discnet::application::configuration_t m_configuration;
        discnet::shared_logger m_logger;
        shared_network_traffic_manager m_network_traffic_manager;
        shared_simulator_adapter_fetcher m_adapter_fetcher;
    };
}