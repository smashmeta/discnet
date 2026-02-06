/*
 *
 */

#pragma once

#include <discnet/typedefs.hpp>
#include <discnet/network/network_handler.hpp>
#include <discnet/adapter_manager.hpp>
#include <discnet/route_manager.hpp>
#include <discnet/network/messages/data_message.hpp>


namespace discnet
{
    using message_t = discnet::network::messages::data_message_t;

    struct recipient_status_t
    {
        discnet::node_identifier_t m_node;
        bool m_sent;
    };

    using recipient_status_list_t = std::vector<recipient_status_t>;

    struct message_status_t
    {
        message_t m_message;
        recipient_status_list_t m_recipients;
    };

    struct message_queue_t
    {
        using recipient_status_list_t = std::vector<recipient_status_t>;
        using message_list_t = std::vector<message_status_t>;
        message_list_t m_message_status_list;
    };

    class transmission_handler
    {
    public:
        DISCNET_EXPORT transmission_handler(const discnet::application::configuration_t& configuration, shared_route_manager route_manager, discnet::network::shared_network_handler network_handler, shared_adapter_manager adapter_manager);

        DISCNET_EXPORT void update(const discnet::time_point_t& current_time);
    private:
        void transmit_discovery_message();
        discnet::network::messages::message_list_t get_messages_for_adapter(const discnet::adapter_identifier_t& adapter_identifier);

        discnet::application::configuration_t m_configuration;
        discnet::shared_logger m_logger;
        discnet::time_point_t m_last_discovery;
        discnet::duration_t m_interval;
        shared_adapter_manager m_adapter_manager;
        shared_route_manager m_route_manager;
        discnet::network::shared_network_handler m_network_handler;
        
        message_queue_t m_queue;
    };
} // !namespace discnet