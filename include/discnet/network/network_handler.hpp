/*
 *
 */

#pragma once

#include "discnet/adapter.hpp"
#include <future>
#include <mutex>
#include <discnet/discnet.hpp>
#include <discnet/adapter_manager.hpp>
#include <discnet/application/configuration.hpp>
#include <discnet/network/udp_client.hpp>
#include <discnet/network/messages/packet.hpp>

namespace discnet::network
{
    struct network_client_t
    {
        discnet::adapter_identifier_t m_adapter_identifier;
        discnet::network::shared_udp_client m_client;
        discnet::network::shared_data_handler m_data_handler;
    };

    // using network_client_t = std::variant<discnet::network::shared_multicast_client, discnet::network::shared_unicast_client>;
    // 
    // struct network_channel_identifier_t
    // {
    //     discnet::adapter_identifier_t m_adapter_identifier;
    //     discnet::endpoint_t m_endpoint;
    // };
    // 
    // struct network_channel_t
    // {
    //     network_channel_identifier_t m_identifier;
    //     network_client_t m_client;
    //     discnet::network::shared_data_handler m_data_handler;
    // };

    struct iclient_creator
    {
        virtual shared_udp_client create(const udp_info_t& info, const data_received_func& callback_func) = 0;
    };

    using shared_client_creator = std::shared_ptr<iclient_creator>;

    struct client_creator : public iclient_creator
    {
        DISCNET_EXPORT client_creator(const discnet::application::configuration_t& configuration, discnet::shared_io_context io_context);

        DISCNET_EXPORT shared_udp_client create(const udp_info_t& info, const data_received_func& callback_func) override;
    private:
        discnet::application::configuration_t m_configuration;
        discnet::shared_logger m_logger;
        discnet::shared_io_context m_io_context;
    };
    
    class network_handler
    {
        using discovery_message_t = discnet::network::messages::discovery_message_t;
        using data_message_t = discnet::network::messages::data_message_t;
        using network_info_t = discnet::network::network_info_t;
        using network_clients_t = std::vector<network_client_t>;
        using data_stream_packets_t = discnet::network::data_stream_packets_t;
        using packet_t = discnet::network::messages::packet_t;
        using message_variant_t = discnet::network::messages::message_variant_t;
        using message_list_t = discnet::network::messages::message_list_t;

        using network_client_result_t = std::expected<network_client_t, std::string>;
        using network_client_future_t = std::future<network_client_result_t>;
        
    public:
        boost::signals2::signal<void(const discovery_message_t&, const network_info_t&)> e_discovery_message_received;
        boost::signals2::signal<void(const data_message_t&, const network_info_t&)> e_data_message_received;
    
    public:
        DISCNET_EXPORT network_handler(const discnet::application::configuration_t& configuration, shared_adapter_manager adapter_manager, shared_client_creator client_creator);
        DISCNET_EXPORT ~network_handler();

        DISCNET_EXPORT network_clients_t clients() const;
        DISCNET_EXPORT void transmit_multicast(const adapter_t& adapter, const message_list_t& messages);
        DISCNET_EXPORT void transmit_unicast(const adapter_t& adapter, const address_t& recipient, message_list_t& messages);
        DISCNET_EXPORT void update(const discnet::time_point_t& current);

    private:
        void remove_client(const adapter_t& adapter);
        void add_client(const adapter_t& adapter);

        network_client_result_t process_adapter(network_client_t client);

        void adapter_added(const adapter_t& adapter);
        void adapter_changed(const adapter_t& previous_adapter, const adapter_t& current_adapter);
        void adapter_removed(const adapter_t& adapter);

        discnet::application::configuration_t m_configuration;
        discnet::shared_logger m_logger;
        discnet::shared_adapter_manager m_adapter_manager;
        shared_client_creator m_client_creator;
        network_clients_t m_clients;

        std::mutex m_adapter_init_list_mutex;
        std::vector<network_client_future_t> m_adapter_init_list;
    };

    using shared_network_handler = std::shared_ptr<network_handler>;
} // ! namespace discnet::network