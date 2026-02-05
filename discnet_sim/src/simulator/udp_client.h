/*
 *
 */

#pragma once

#include <discnet/network/udp_client.hpp>


namespace discnet::sim::logic
{
    class network_traffic_manager;
    using shared_network_traffic_manager = std::shared_ptr<network_traffic_manager>;

    class simulator_udp_client : public network::iudp_client
    {
    public:
        simulator_udp_client(const uint16_t node_id, const shared_network_traffic_manager& ntm, const network::udp_info_t& info, 
            const network::data_received_func& func);
    
        bool open() override;
        bool write(const discnet::network::buffer_t& buffer) override;
        bool write(const discnet::network::buffer_t& buffer, const discnet::address_t& recipient) override;
        void close() override;
        
        void receive_bytes(const discnet::network::buffer_t& buffer, const discnet::address_t& sender);

    private:
        uint16_t m_node_id;
        shared_network_traffic_manager m_network_traffic_manager;
    };

    struct simulator_client_creator : public discnet::network::iclient_creator
    {
        simulator_client_creator(const discnet::application::shared_loggers& loggers, const uint16_t node_id, const shared_network_traffic_manager& ntm);
        discnet::network::shared_udp_client create(const discnet::network::udp_info_t& info, const discnet::network::data_received_func& callback_func) override;

    private:
        discnet::application::shared_loggers m_loggers;
        uint16_t m_node_id;
        shared_network_traffic_manager m_network_traffic_manager;
    };
}