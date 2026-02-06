/*
 *
 */

#include <discnet/transmission_handler.hpp>


namespace discnet
{
    transmission_handler::transmission_handler(const discnet::application::configuration_t& configuration, shared_route_manager route_manager, discnet::network::shared_network_handler network_handler, shared_adapter_manager adapter_manager)
        : m_configuration(configuration),
        m_logger(spdlog::get(configuration.m_log_instance_id)),
        m_last_discovery(discnet::time_point_t::min()),
        m_interval(std::chrono::seconds(20)),     
        m_adapter_manager(adapter_manager),
        m_route_manager(route_manager), 
        m_network_handler(network_handler)
    {
        // nothing for now
    }

    void transmission_handler::update(const discnet::time_point_t& current_time)
    {
        auto next_discovery_time = m_last_discovery + m_interval;
        if (next_discovery_time < current_time)
        {
            transmit_discovery_message();
            m_last_discovery = current_time;
            return;
        }

        auto diff_forward = current_time - m_last_discovery;
        if (diff_forward > m_interval)
        {
            // system clock has been moved forward. 
            // we distribute discovery message to correct internal timing. 
            transmit_discovery_message();
            m_last_discovery = current_time;
        }
    }

    void transmission_handler::transmit_discovery_message()
    {
        auto clients = m_network_handler->clients();
        for (discnet::network::network_client_t& client : clients)
        {
            auto adapter = m_adapter_manager->find_adapter(client.m_adapter_identifier);
            if (adapter)
            {
                m_logger->info("sending discovery message on adapter: {}.", adapter->m_name);
                
                discnet::network::messages::discovery_message_t discovery {.m_identifier = m_configuration.m_node_id};
                auto routes = m_route_manager->find_routes_for_adapter(adapter->m_guid);
                for (const auto& route : routes)
                {
                    discnet::network::messages::node_t indirect_node {.m_identifier = route.m_identifier.m_node.m_id, .m_address = route.m_identifier.m_node.m_address};
                    indirect_node.m_jumps = route.m_status.m_jumps;
                    indirect_node.m_jumps.push_back(256); // adding self
                    discovery.m_nodes.push_back(indirect_node);
                }
                
                discnet::network::messages::message_list_t messages {discovery};
                m_network_handler->transmit_multicast(*adapter, messages);
            }
        }
    }

    discnet::network::messages::message_list_t transmission_handler::get_messages_for_adapter(const discnet::adapter_identifier_t& adapter_identifier)
    {
        boost::ignore_unused(adapter_identifier);

        size_t remaining_packet_size = 1024;
        discnet::network::messages::message_list_t messages;
        for (auto& status : m_queue.m_message_status_list)
        {
            size_t message_size = status.m_message.m_buffer.size();
            if (message_size <= remaining_packet_size)
            {
                for (auto& recipient : status.m_recipients)
                {
                    // if not sent and best route is given adapter
                    if (!recipient.m_sent && recipient.m_node.m_id)
                    {
                        recipient.m_sent = true;
                        messages.push_back(status.m_message);
                        remaining_packet_size -= message_size;
                    }
                }
            }
            else
            {
                // skipping message because it is too big 
            }

            if (remaining_packet_size == 0)
            {
                break;
            }
        }

        return messages;
    }
} // !namespace discnet