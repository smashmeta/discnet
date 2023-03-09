/*
 *
 */

#include <iostream>
#include <map>
#include <chrono>
#include <ranges>
#include <boost/core/ignore_unused.hpp>
#include <boost/asio.hpp>
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


namespace discnet::main
{
    using shared_network_handler = discnet::network::shared_network_handler;

    void work_handler(const std::string& thread_name, discnet::shared_io_context io_context)
    {
        whatlog::rename_thread(GetCurrentThread(), thread_name);
        whatlog::logger log("work_handler");
        log.info("starting thread {}.", thread_name);

        for (;;)
        {
            try
            {
                boost::system::error_code error_code;
                io_context->run(error_code);
                if (error_code)
                {
                    log.error("worker thread encountered an error. message: ", error_code.message());
                }

                break;
            }
            catch (std::exception& ex)
            {
                log.warning("worker thread encountered an error. exception: {}.", std::string(ex.what()));
            }
            catch (...)
            {
                log.warning("worker thread encountered an unknown exception.");
            }
        }
    }
    
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
}

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

    std::vector<std::string> thread_names = {"mercury", "venus"};
    const size_t worker_threads_count = thread_names.size();

    auto io_context = std::make_shared<boost::asio::io_context>((int)worker_threads_count);
    auto work = std::make_shared<boost::asio::io_context::work>(*io_context);
    
    boost::thread_group worker_threads;
    for (size_t i = 0; i < worker_threads_count; ++i)
    {
        worker_threads.create_thread(boost::bind(&discnet::main::work_handler, thread_names[i], io_context));
    }

    auto adapter_manager = std::make_shared<discnet::adapter_manager>(std::move(std::make_unique<discnet::windows_adapter_fetcher>()));
    auto route_manager = std::make_shared<discnet::route_manager>(adapter_manager);
    auto network_handler = std::make_shared<discnet::network::network_handler>(adapter_manager, configuration.value(), io_context);
    auto discovery_handler = std::make_shared<discnet::main::discovery_message_handler>(network_handler, route_manager, adapter_manager);
    auto transmission_handler = std::make_shared<discnet::main::transmission_handler>(route_manager, network_handler, adapter_manager, configuration.value());

    while (true)
    {
        std::chrono::time_point current_time = discnet::time_point_t::clock::now();
        adapter_manager->update();
        network_handler->update();
        route_manager->update(current_time);
        transmission_handler->update(current_time);

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    return EXIT_SUCCESS;
}