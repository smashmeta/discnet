/*
 *
 */

#include <iostream>
#include <map>
#include <chrono>
#include <ranges>
#include <boost/core/ignore_unused.hpp>
#include <boost/thread.hpp>
#include <boost/dll.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <discnet/discnet.hpp>
#include <whatlog/logger.hpp>
#include <discnet/node.hpp>
#include <discnet/route_manager.hpp>
#include <discnet/network/buffer.hpp>
#include <discnet/network/messages/data_message.hpp>
#include <discnet/network/messages/discovery_message.hpp>
#include <discnet/network/messages/packet.hpp>
#include <discnet/network/network_handler.hpp>
#include <discnet/network/data_handler.hpp>
#include <discnet/adapter_manager.hpp>
#include <discnet/application/configuration.hpp>
#include <discnet_app/asio_context.hpp>


namespace discnet::main
{
    using shared_network_handler = discnet::network::shared_network_handler;

    class discovery_message_handler
    {
        typedef discnet::network::messages::discovery_message_t discovery_message_t;
        typedef discnet::network::network_info_t network_info_t;
        typedef discnet::shared_route_manager shared_route_manager;
    public:
        discovery_message_handler(shared_network_handler network_handler, shared_route_manager route_manager, shared_adapter_manager adapter_manager)
            : m_route_manager(route_manager), m_adapter_manager(adapter_manager)
        {
            network_handler->e_discovery_message_received.connect(
                std::bind(&discovery_message_handler::handle_discovery_message, this, std::placeholders::_1, std::placeholders::_2));
        }

        void handle_discovery_message(const discovery_message_t& message, const network_info_t& network_info)
        {
            m_route_manager->process(network_info, message);
        }
    private:
        shared_route_manager m_route_manager;
        shared_adapter_manager m_adapter_manager;
    };

    typedef std::shared_ptr<discovery_message_handler> shared_discovery_message_handler;

    class transmission_handler
    {
        typedef discnet::shared_route_manager shared_route_manager;
    public:
        transmission_handler(shared_route_manager route_manager, shared_network_handler multicast_hndlr, shared_adapter_manager adapter_manager, discnet::application::configuration_t configuration)
            : m_route_manager(route_manager), m_multicast_handler(multicast_hndlr), m_adapter_manager(adapter_manager),
                m_last_discovery(discnet::time_point_t::clock::from_time_t(0)), m_interval(std::chrono::seconds(20)), m_configuration(configuration)
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
            whatlog::logger log("transmission_handler::transmit_discovery_message");
            auto adapters = m_adapter_manager->adapters();
            for (discnet::adapter_t& adapter : adapters)
            {
                if (adapter.m_enabled && adapter.m_multicast_enabled)
                {
                    log.info("sending discovery message on adapter: {}.", adapter.m_name);
                    
                    discnet::network::messages::discovery_message_t discovery {.m_identifier = m_configuration.m_node_id };
                    auto routes = m_route_manager->find_routes_for_adapter(adapter.m_guid);
                    for (const auto& route : routes)
                    {
                        discnet::network::messages::node_t indirect_node {.m_identifier = route.m_identifier.m_node.m_id, .m_address = route.m_identifier.m_node.m_address};
                        indirect_node.m_jumps = route.m_status.m_jumps;
                        indirect_node.m_jumps.push_back(256); // adding self
                        discovery.m_nodes.push_back(indirect_node);
                    }
                    
                    discnet::network::messages::message_list_t messages { discovery };
                    m_multicast_handler->transmit_multicast(adapter, messages);
                }
            }
        }

        discnet::time_point_t m_last_discovery;
        discnet::duration_t m_interval;
        shared_adapter_manager m_adapter_manager;
        shared_route_manager m_route_manager;
        shared_network_handler m_multicast_handler;
        discnet::application::configuration_t m_configuration;
    };
} // ! namespace discnet::main

/*
 *  - persistent node(s)
 *  - transmission_handler: queue
 *  - data_message_handler: message processing
 *  - data_message_handler: message routing
 *  - data_message_t: destination(s)
 *  - data_message_t: replace
 */

int main(int arguments_count, const char** arguments_vector)
{
    whatlog::rename_thread(GetCurrentThread(), "main");
    if (!discnet::application::initialize_console_logger())
    {
        return EXIT_FAILURE;
    }

    boost::filesystem::path executable_directory = boost::dll::program_location().parent_path();
    std::cout << "current path is set to " << executable_directory << std::endl;
    whatlog::logger::initialize_file_logger(executable_directory, "discnet");
    whatlog::logger log("main");

    discnet::application::expected_configuration_t configuration = discnet::application::get_configuration(arguments_count, arguments_vector);
    if (!configuration)
    {
        log.error("failed to load configuration. terminating application.");
        return EXIT_FAILURE;
    }

    log.info("configuration loaded. node_id: {}, mc-address: {}, mc-port: {}.", 
        configuration->m_node_id, configuration->m_multicast_address.to_string(), configuration->m_multicast_port);
    
    log.info("setting up asio network context...");
    auto asio_context = std::make_shared<discnet::main::asio_context_t>();
    log.info("setting up adapter_manager...");
    auto adapter_manager = std::make_shared<discnet::adapter_manager>(std::move(std::make_unique<discnet::windows_adapter_fetcher>()));
    log.info("setting up route_manager...");
    auto route_manager = std::make_shared<discnet::route_manager>(adapter_manager);
    log.info("setting up network_handler...");
    auto network_handler = std::make_shared<discnet::network::network_handler>(adapter_manager, configuration.value(), asio_context->m_io_context);
    log.info("setting up discovery_handler...");
    auto discovery_handler = std::make_shared<discnet::main::discovery_message_handler>(network_handler, route_manager, adapter_manager);
    log.info("setting up transmission_handler...");
    auto transmission_handler = std::make_shared<discnet::main::transmission_handler>(route_manager, network_handler, adapter_manager, configuration.value());

    log.info("discnet initialized and running.");
    while (true)
    {
        auto current_time = discnet::time_point_t::clock::now();

        adapter_manager->update();
        network_handler->update();
        route_manager->update(current_time);
        transmission_handler->update(current_time);

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    return EXIT_SUCCESS;
}