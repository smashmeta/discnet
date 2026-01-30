/*
 *
 */

#include <spdlog/spdlog.h>
#include <spdlog/sinks/qt_sinks.h>
#include <discnet/application/configuration.hpp>
#include <discnet_app/asio_context.hpp>
#include <discnet/adapter_manager.hpp>
#include <discnet/route_manager.hpp>
#include <discnet/network/network_handler.hpp>
#include "node.h"

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

    class discovery_message_handler
    {
        typedef discnet::network::messages::discovery_message_t discovery_message_t;
        typedef discnet::network::network_info_t network_info_t;
        typedef discnet::shared_route_manager shared_route_manager;
    public:
        discovery_message_handler(discnet::network::shared_network_handler network_handler, shared_route_manager route_manager)
            : m_route_manager(route_manager)
        {
            network_handler->e_discovery_message_received.connect(
                std::bind(&discovery_message_handler::handle_discovery_message, this, std::placeholders::_1, std::placeholders::_2));
        }

        void handle_discovery_message(const discovery_message_t& message, const network_info_t& network_info)
        {
            m_route_manager->process_discovery_message(message, network_info);
        }
    private:
        shared_route_manager m_route_manager;
    };

    class transmission_handler
    {
        typedef discnet::shared_route_manager shared_route_manager;
    public:
        transmission_handler(const discnet::application::shared_loggers& loggers, shared_route_manager route_manager, discnet::network::shared_network_handler network_handler, shared_adapter_manager adapter_manager, discnet::application::configuration_t configuration)
            : m_last_discovery(discnet::time_point_t::min()),
            m_interval(std::chrono::seconds(20)),     
            m_loggers(loggers),
            m_adapter_manager(adapter_manager),
            m_route_manager(route_manager), 
            m_network_handler(network_handler), 
            m_configuration(configuration)
        {
            // nothing for now
        }

        void update(const discnet::time_point_t& current_time)
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
    private:
        void transmit_discovery_message()
        {
            auto clients = m_network_handler->clients();
            for (discnet::network::network_client_t& client : clients)
            {
                auto adapter = m_adapter_manager->find_adapter(client.m_adapter_identifier);
                if (adapter)
                {
                    m_loggers->m_logger->info("sending discovery message on adapter: {}.", adapter->m_name);
                    
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
        
        discnet::network::messages::message_list_t get_messages_for_adapter(const discnet::adapter_identifier_t& adapter_identifier)
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

        discnet::time_point_t m_last_discovery;
        discnet::duration_t m_interval;
        discnet::application::shared_loggers m_loggers;
        shared_adapter_manager m_adapter_manager;
        shared_route_manager m_route_manager;
        discnet::network::shared_network_handler m_network_handler;
        discnet::application::configuration_t m_configuration;
        message_queue_t m_queue;
    };

    class simulator_multicast_client : public network::imulticast_client
    {
    public:
        simulator_multicast_client(const uint16_t node_id, const shared_network_traffic_manager& ntm, const network::multicast_info_t& info, const network::data_received_func& func)
            : network::imulticast_client(info, func), m_node_id(node_id), m_network_traffic_manager(ntm)
        {
            // nothing for now
        }
    
        bool open() override { return true; }
        bool write(const discnet::network::buffer_t& buffer) override 
        { 
            m_network_traffic_manager->data_sent(m_node_id, m_info, buffer);
            return true; 
        }
        void close() override { }
        network::multicast_info_t info() const override { return m_info; }

        void receive_bytes(const discnet::network::buffer_t& buffer, const discnet::address_t& sender)
        {
            m_data_received_func(buffer.const_buffer(), sender, m_info.m_multicast_address);
        }

    private:
        uint16_t m_node_id;
        shared_network_traffic_manager m_network_traffic_manager;
    };

    class simulator_unicast_client : public network::iunicast_client
    {
    public:
        simulator_unicast_client(const uint16_t node_id, const shared_network_traffic_manager& ntm, const network::unicast_info_t& info, const network::data_received_func& func)
            : network::iunicast_client(info, func), m_node_id(node_id), m_network_traffic_manager(ntm)
        {
            // nothing for now
        }
    
        bool open() override { return true; }
        bool write([[maybe_unused]] const discnet::address_t& recipient, [[maybe_unused]] const discnet::network::buffer_t& buffe) override { return false; }
        void close() override { }

    private:
        uint16_t m_node_id;
        shared_network_traffic_manager m_network_traffic_manager;
    };

    struct simulator_client_creator : public discnet::network::iclient_creator
    {
        simulator_client_creator(const discnet::application::shared_loggers& loggers, const uint16_t node_id, const shared_network_traffic_manager& ntm)
            : m_loggers(loggers), m_node_id(node_id), m_network_traffic_manager(ntm)
        {
            // nothing for now
        }

        discnet::network::shared_multicast_client create(const discnet::network::multicast_info_t& info, const discnet::network::data_received_func& callback_func) override 
        { 
            auto result = std::make_shared<simulator_multicast_client>(m_node_id, m_network_traffic_manager, info, callback_func);
            m_network_traffic_manager->register_client(m_node_id, result);
            return result; 
        }

        discnet::network::shared_unicast_client create(const discnet::network::unicast_info_t& info, const discnet::network::data_received_func& callback_func) override 
        { 
            auto result = std::make_shared<simulator_unicast_client>(m_node_id, m_network_traffic_manager, info, callback_func); 
            return result;
        }
    private:
        discnet::application::shared_loggers m_loggers;
        uint16_t m_node_id;
        shared_network_traffic_manager m_network_traffic_manager;
    };

    void network_traffic_manager::data_sent(const uint16_t node_id, const network::multicast_info_t& info, const discnet::network::buffer_t& buffer)
    {
        if (m_network_logger)
        {
            std::string log_entry = std::format("node: {}, adapter: {}, multicast: [addr: {}, port: {}] - {}",
                node_id, info.m_adapter_address.to_string(), info.m_multicast_address.to_string(), info.m_multicast_port, discnet::bytes_to_hex_string(buffer.data()));
            m_network_logger->info(log_entry);

            std::string subnet_mask_sender;
            {
                auto ip_str = info.m_adapter_address.to_string();
                auto last_segment_pos = ip_str.find_last_of('.');
                subnet_mask_sender = ip_str.substr(0, last_segment_pos);
            }

            for (auto& [id, adapters] : m_mc_clients)
            {
                if (id != node_id)
                {
                    for (auto& adapter : adapters)
                    {
                        auto sim_adapter = std::dynamic_pointer_cast<simulator_multicast_client>(adapter);
                        if (sim_adapter)
                        {
                            std::string subnet_mask_receiver;
                            {
                                auto ip_str = adapter->info().m_adapter_address.to_string();
                                auto last_segment_pos = ip_str.find_last_of('.');
                                subnet_mask_receiver = ip_str.substr(0, last_segment_pos);
                            }

                            if (subnet_mask_sender == subnet_mask_receiver)
                            {
                                sim_adapter->receive_bytes(buffer, info.m_adapter_address);
                            }
                        }
                    }
                }
            }
        }
    }

    static int instance_id = 0;
    discnet_node::discnet_node(const application::configuration_t& configuration, const shared_network_traffic_manager& ntm, QTextEdit* text_edit)
        : m_configuration(configuration), m_network_traffic_manager(ntm)
    {
        // nothing for now
        m_loggers = std::make_shared<discnet::application::loggers_t>();
        m_loggers->m_logger = spdlog::qt_logger_mt(std::format("qt_{}", instance_id++), text_edit);
    }

    discnet_node::~discnet_node()
    {
        m_loggers->m_logger->info("shutting down discnet node [{}].", m_configuration.m_node_id);
    }

    void discnet_node::add_adapter(const adapter_t& adapter)
    {
        m_adapter_fetcher->add_adapter(adapter);
    }

    bool discnet_node::initialize()
    {
        m_loggers->m_logger->info(std::format("discnet - node id: {}, multicast: [addr: {}, port: {}].", 
            m_configuration.m_node_id, m_configuration.m_multicast_address.to_string(), m_configuration.m_multicast_port));

        m_loggers->m_logger->info("setting up adapter_fetcher...");
        m_adapter_fetcher = std::make_shared<discnet::simulator_adapter_fetcher>();
        m_loggers->m_logger->info("setting up adapter_manager...");
        m_adapter_manager = std::make_shared<discnet::adapter_manager>(m_loggers, m_adapter_fetcher);

        m_loggers->m_logger->info("setting up network_handler...");
        m_network_handler = std::make_shared<discnet::network::network_handler>(m_loggers, m_adapter_manager, m_configuration, std::make_shared<simulator_client_creator>(m_loggers, m_configuration.m_node_id, m_network_traffic_manager));
        m_loggers->m_logger->info("setting up route_manager...");
        m_route_manager = std::make_shared<discnet::route_manager>(m_loggers, m_adapter_manager, m_network_handler);
        m_loggers->m_logger->info("setting up transmission_handler...");
        m_transmission_handler = std::make_shared<transmission_handler>(m_loggers, m_route_manager, m_network_handler, m_adapter_manager, m_configuration);

        return true;
    }

    void discnet_node::update(discnet::time_point_t current_time)
    {
        m_adapter_manager->update();
        m_network_handler->update();
        m_route_manager->update(current_time);
        m_transmission_handler->update(current_time);
    }
} // !namespace discnet::main