/*
 *
 */

#include <whatlog/logger.hpp>
#include <discnet/application/configuration.hpp>
#include <discnet_app/asio_context.hpp>
#include <discnet/adapter_manager.hpp>
#include <discnet/route_manager.hpp>
#include <discnet/network/network_handler.hpp>
#include <discnet_app/application.hpp>


namespace discnet::main
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
        discovery_message_handler(discnet::network::shared_network_handler network_handler, shared_route_manager route_manager, shared_adapter_manager adapter_manager)
            : m_route_manager(route_manager), m_adapter_manager(adapter_manager)
        {
            network_handler->e_discovery_message_received.connect(
                std::bind(&discovery_message_handler::handle_discovery_message, this, std::placeholders::_1, std::placeholders::_2));
        }

        void handle_discovery_message(const discovery_message_t& message, const network_info_t& network_info)
        {
            whatlog::logger log("discovery_message_handler::handle_discovery_message");
            m_route_manager->process(message, network_info);
        }
    private:
        shared_route_manager m_route_manager;
        shared_adapter_manager m_adapter_manager;
    };

    class transmission_handler
    {
        typedef discnet::shared_route_manager shared_route_manager;
    public:
        transmission_handler(shared_route_manager route_manager, discnet::network::shared_network_handler multicast_handler, shared_adapter_manager adapter_manager, discnet::application::configuration_t configuration)
            : m_route_manager(route_manager), m_network_handler(multicast_handler), m_adapter_manager(adapter_manager),
                m_last_discovery(discnet::time_point_t::min()), m_interval(std::chrono::seconds(20)), m_configuration(configuration)
        {
            // nothing for now
        }

        void update(const discnet::time_point_t& current_time)
        {
            auto adapters = m_adapter_manager->adapters();
            for (auto& adapter : adapters)
            {
                auto messages = get_messages_for_adapter(adapter.m_guid);
                if (messages.size() > 0)
                {
                    m_network_handler->transmit_multicast(adapter, messages);
                }
            }

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
            whatlog::logger log("transmission_handler::transmit_discovery_message");
            auto adapters = m_adapter_manager->adapters();
            for (discnet::adapter_t& adapter : adapters)
            {
                if (adapter.m_enabled && adapter.m_multicast_present)
                {
                    log.info("sending discovery message on adapter: {}.", adapter.m_name);
                    
                    discnet::network::messages::discovery_message_t discovery {.m_identifier = m_configuration.m_node_id};
                    auto routes = m_route_manager->find_routes_for_adapter(adapter.m_guid);
                    for (const auto& route : routes)
                    {
                        discnet::network::messages::node_t indirect_node {.m_identifier = route.m_identifier.m_node.m_id, .m_address = route.m_identifier.m_node.m_address};
                        indirect_node.m_jumps = route.m_status.m_jumps;
                        indirect_node.m_jumps.push_back(256); // adding self
                        discovery.m_nodes.push_back(indirect_node);
                    }
                    
                    discnet::network::messages::message_list_t messages {discovery};
                    m_network_handler->transmit_multicast(adapter, messages);
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
        shared_adapter_manager m_adapter_manager;
        shared_route_manager m_route_manager;
        discnet::network::shared_network_handler m_network_handler;
        discnet::application::configuration_t m_configuration;
        message_queue_t m_queue;
    };
}

namespace discnet::main
{
    application::application(discnet::application::configuration_t configuration)
        : m_configuration(configuration)
    {
        // nothing for now
    }

    bool application::initialize()
    {
        whatlog::logger log("application::initialize");

        log.info("setting up asio network context...");
        m_asio_context = std::make_shared<discnet::main::asio_context_t>();
        log.info("setting up adapter_manager...");
        m_adapter_manager = std::make_shared<discnet::adapter_manager>(std::move(std::make_unique<discnet::windows_adapter_fetcher>()));
        log.info("setting up route_manager...");
        m_route_manager = std::make_shared<discnet::route_manager>(adapter_manager);
        log.info("setting up network_handler...");
        m_network_handler = std::make_shared<discnet::network::network_handler>(adapter_manager, m_configuration, m_asio_context->m_io_context);
        log.info("setting up discovery_handler...");
        m_discovery_handler = std::make_shared<discnet::main::discovery_message_handler>(network_handler, route_manager, adapter_manager);
        log.info("setting up transmission_handler...");
        m_transmission_handler = std::make_shared<discnet::main::transmission_handler>(route_manager, network_handler, adapter_manager, m_configuration);

        return true;
    }

    void application::update(discnet::time_point_t current_time)
    {
        m_adapter_manager->update();
        m_network_handler->update();
        m_route_manager->update(current_time);
        m_transmission_handler->update(current_time);
    }
} // !namespace discnet::main