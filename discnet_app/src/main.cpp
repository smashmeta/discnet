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
#include <discnet/network/multicast_client.hpp>
#include <discnet/network/unicast_client.hpp>
#include <discnet/network/data_handler.hpp>
#include <discnet/adapter_manager.hpp>
#include <discnet_app/configuration.hpp>


namespace discnet::app
{
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

    struct udp_client
    {
        discnet::network::shared_multicast_client m_multicast;
        discnet::network::shared_unicast_client m_unicast;
        discnet::network::shared_data_handler m_data_handler;
    };

    class multicast_handler
    {
        typedef discnet::network::messages::discovery_message_t discovery_message_t;
        typedef discnet::network::messages::data_message_t data_message_t;
        typedef discnet::network::network_info_t network_info_t;
        typedef std::map<boost::uuids::uuid, udp_client> udp_client_map_t;
        
    public:
        boost::signals2::signal<void(const discovery_message_t&, const network_info_t&)> e_discovery_message_received;
        boost::signals2::signal<void(const data_message_t&, const network_info_t&)> e_data_message_received;
    
    public:
        multicast_handler(shared_adapter_manager adapter_manager, const discnet::app::configuration_t& configuration, discnet::shared_io_context io_context)
            : m_adapter_manager(adapter_manager), m_configuration(configuration), m_io_context(io_context)
        {
            m_adapter_manager->e_new.connect(std::bind(&multicast_handler::adapter_added, this, std::placeholders::_1));
            m_adapter_manager->e_changed.connect(std::bind(&multicast_handler::adapter_changed, this, std::placeholders::_1, std::placeholders::_2));
            m_adapter_manager->e_removed.connect(std::bind(&multicast_handler::adapter_removed, this, std::placeholders::_1));
        }

        void transmit_multicast(const discnet::adapter_t& adapter, const discnet::network::messages::message_list_t& messages)
        {
            whatlog::logger log("multicast_handler::transmit_multicast");
            discnet::network::buffer_t buffer(4096);
            auto success = discnet::network::messages::packet_codec_t::encode(buffer, messages);
            if (!success)
            {
                log.error("failed to encode messages to a valid packet.");
            }

            auto itr_client = m_clients.find(adapter.m_guid);
            if (itr_client != m_clients.end())
            {
                auto& [uuid, udp_client] = *itr_client;
                udp_client.m_multicast->write(buffer);
            }
        }

        void transmit_unicast(const discnet::adapter_t& adapter, const::discnet::address_t& recipient, discnet::network::messages::message_list_t& messages)
        {
            whatlog::logger log("multicast_handler::transmit_unicast");
            discnet::network::buffer_t buffer(4096);
            auto success = discnet::network::messages::packet_codec_t::encode(buffer, messages);
            if (!success)
            {
                log.error("failed to encode messages to a valid packet.");
            }

            auto itr_client = m_clients.find(adapter.m_guid);
            if (itr_client != m_clients.end())
            {
                auto& [uuid, udp_client] = *itr_client;
                udp_client.m_unicast->write(recipient, buffer);
            }
        }

        void update()
        {
            using data_stream_packets_t = discnet::network::data_stream_packets_t;
            using packet_t = discnet::network::messages::packet_t;
            using discovery_message_t = discnet::network::messages::discovery_message_t;
            using data_message_t = discnet::network::messages::data_message_t;
            using multicast_info_t = discnet::network::multicast_info_t;
            using message_variant_t = discnet::network::messages::message_variant_t;

            whatlog::logger log("multicast_handler::update");
            for (auto& client : m_clients | std::views::values)
            {
                network_info_t network_info;
                network_info.m_adapter = client.m_multicast->info().m_adapter_address;
                network_info.m_receiver = client.m_multicast->info().m_multicast_address;
                network_info.m_reception_time = discnet::time_point_t::clock::now();

                auto streams = client.m_data_handler->process();
                for (const data_stream_packets_t& stream : streams)
                {
                    for (const packet_t& packet : stream.m_packets)
                    {
                        network_info.m_sender = stream.m_identifier.m_sender_ip;
                        network_info.m_receiver = stream.m_identifier.m_recipient_ip;

                        for (const message_variant_t& message : packet.m_messages)
                        {
                            if (std::holds_alternative<discovery_message_t>(message))
                            {
                                auto discovery_message = std::get<discovery_message_t>(message);
                                e_discovery_message_received(discovery_message, network_info);
                                discovery_message_received(discovery_message, network_info);
                            }
                            else if (std::holds_alternative<data_message_t>(message))
                            {
                                auto data_message = std::get<data_message_t>(message);
                                e_data_message_received(data_message, network_info);
                                data_message_received(data_message, network_info);
                            }
                        }
                    }
                }
            }
        }

    private:
        void remove_client(const discnet::adapter_t& adapter)
        {
            whatlog::logger log("multicast_handler::remove_multicast_client");

            auto itr_client = m_clients.find(adapter.m_guid);
            if (itr_client != m_clients.end())
            {
                auto& [uuid, client] = *itr_client;
                std::string adapter_guid_str = boost::lexical_cast<std::string>(uuid);
                log.info("removing adapter (name: {}, guid: {}).", adapter.m_name, adapter_guid_str);
                client.m_multicast->close();
                m_clients.erase(itr_client);
            }
        }
        
        void add_client(const discnet::adapter_t& adapter)
        {
            whatlog::logger log("multicast_handler::add_multicast_client");

            if (!adapter.m_enabled)
            {
                log.info("skipping adapter {} because it is disabled.", adapter.m_name);
                return;
            }

            if (adapter.m_address_list.empty())
            {
                log.info("skipping adapter {} because it is missing ipv4 address.", adapter.m_name);
                return;
            }

            discnet::network::multicast_info_t multicast_info;
            multicast_info.m_adapter_address = adapter.m_address_list.front().first;
            multicast_info.m_multicast_address = m_configuration.m_multicast_address;
            multicast_info.m_multicast_port = m_configuration.m_multicast_port;

            discnet::network::unicast_info_t unicast_info;
            unicast_info.m_address = adapter.m_address_list.front().first;
            unicast_info.m_port = m_configuration.m_multicast_port + 1;

            udp_client client;
            client.m_data_handler = std::make_shared<discnet::network::data_handler>(4095);
            client.m_multicast = discnet::network::multicast_client::create(m_io_context, multicast_info, client.m_data_handler);
            client.m_unicast = discnet::network::unicast_client::create(m_io_context, unicast_info, client.m_data_handler);

            bool unicast_enabled = client.m_unicast->open();
            bool multicast_enabled = client.m_multicast->open();
            size_t index = 1;
            discnet::time_point_t start_time = discnet::time_point_t::clock::now();
            discnet::time_point_t current_time = start_time;
            discnet::time_point_t timeout = start_time + std::chrono::seconds(15); 
            while ((!unicast_enabled || !multicast_enabled) && current_time < timeout)
            {
                log.info("retry connect #{}.", index);
                // give the OS some time to initialize the adapter before we start listening
                std::this_thread::sleep_for(std::chrono::seconds(1));
                if (!multicast_enabled)
                {
                    multicast_enabled = client.m_multicast->open();
                }
                if (!unicast_enabled)
                {
                    unicast_enabled = client.m_unicast->open();
                }
                
                current_time = discnet::time_point_t::clock::now();
                ++index;
            }

            if (!multicast_enabled)
            {
                log.error("failed to start multicast client. dropping client for adapter.");
                return;
            }

            if (!unicast_enabled)
            {
                log.error("failed to start unicast client. dropping client for adapter");
                return;            
            }

            log.info("Added adapater to our client map.", multicast_info.m_adapter_address.to_string());
            m_clients.insert(std::pair{adapter.m_guid, client});
        }

        void adapter_added(const adapter_t& adapter)
        {
            whatlog::logger log("multicast_handler::adapter_added");
            
            std::string adapter_guid_str = boost::lexical_cast<std::string>(adapter.m_guid);
            log.info("new adapter detected. Name: {}, guid: {}, mac: {}.", adapter.m_name, adapter_guid_str, adapter.m_mac_address);
            add_client(adapter);
        }

        void adapter_changed(const adapter_t& previous_adapter, const adapter_t& current_adapter)
        {
            whatlog::logger log("multicast_handler::adapter_changed");
            
            bool multicast_client_exist = m_clients.find(current_adapter.m_guid) != m_clients.end();
            if (multicast_client_exist)
            {
                bool ip_address_changed = previous_adapter.m_address_list != current_adapter.m_address_list;
                if (!current_adapter.m_multicast_enabled)
                {
                    log.info("adapter {} multicast disabled. removing multicast client.", current_adapter.m_name);
                    remove_client(previous_adapter);
                }
                else if (ip_address_changed)
                {
                    log.info("adapter {} ip-address chaned. re-creating multicast client.", current_adapter.m_name);
                    remove_client(previous_adapter);
                    add_client(current_adapter);
                }
            }
            else 
            {
                if (current_adapter.m_multicast_enabled)
                {
                    log.info("unknown adapter {} appeared. adding multicast client.", current_adapter.m_name);
                    add_client(current_adapter);
                }
            }
        }

        void adapter_removed(const adapter_t& adapter)
        {
            whatlog::logger log("multicast_handler::adapter_removed");
            remove_client(adapter);
        }

        void discovery_message_received(const discovery_message_t& message, const network_info_t& network_info)
        {
            boost::ignore_unused(message);
            auto itr_adapter = m_adapter_manager->find_adapter(network_info.m_adapter);
            if (itr_adapter)
            {
                static uint16_t identifier = 1;
                discnet::network::messages::data_message_t data_message {.m_identifier = identifier++};
                data_message.m_buffer = { 1,2,3,4,5 };
                discnet::network::messages::message_list_t messages{data_message};
                transmit_unicast(*itr_adapter, network_info.m_sender, messages);
            }
        }

        void data_message_received(const data_message_t& message, const network_info_t& network_info)
        {
            whatlog::logger log("multicast_handler::data_message_received");
            std::string message_str {(char*)message.m_buffer.data(), message.m_buffer.size()};
            log.info("received data_message (id: {}, msg: {}) from {}. receive address {}.", message.m_identifier, message_str, network_info.m_sender.to_string(), network_info.m_receiver.to_string());
        }
        
        discnet::shared_adapter_manager m_adapter_manager;
        discnet::app::configuration_t m_configuration;
        discnet::shared_io_context m_io_context;
        udp_client_map_t m_clients;
    };

    typedef std::shared_ptr<multicast_handler> shared_multicast_handler;
    
    class discovery_message_handler
    {
        typedef discnet::network::messages::discovery_message_t discovery_message_t;
        typedef discnet::network::network_info_t network_info_t;
        typedef discnet::shared_route_manager shared_route_manager;
    public:
        discovery_message_handler(shared_multicast_handler multicast_handler, shared_route_manager route_manager, shared_adapter_manager adapter_manager)
            : m_route_manager(route_manager), m_adapter_manager(adapter_manager)
        {
            multicast_handler->e_discovery_message_received.connect(
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
        transmission_handler(shared_route_manager route_manager, shared_multicast_handler multicast_hndlr, shared_adapter_manager adapter_manager, discnet::app::configuration_t configuration)
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
        shared_multicast_handler m_multicast_handler;
        discnet::app::configuration_t m_configuration;
    };
}

int main(int arguments_count, const char** arguments_vector)
{
    whatlog::rename_thread(GetCurrentThread(), "main");
    if (!discnet::app::initialize_console_logger())
    {
        return EXIT_FAILURE;
    }

    boost::filesystem::path executable_directory = boost::dll::program_location().parent_path();
    std::cout << "current path is set to " << executable_directory << std::endl;
    whatlog::logger::initialize_file_logger(executable_directory, "discnet");
    whatlog::logger log("main");

    auto configuration = discnet::app::get_configuration(arguments_count, arguments_vector);
    if (!configuration)
    {
        log.error("failed to load configuration. terminating application.");
        return EXIT_FAILURE;
    }

    log.info("configuration loaded. node_id: {}, mc-address: {}, mc-port: {}", 
        configuration->m_node_id, configuration->m_multicast_address.to_string(), configuration->m_multicast_port);

    std::vector<std::string> thread_names = { "mercury", "venus" };
    const size_t worker_threads_count = thread_names.size();

    auto io_context = std::make_shared<boost::asio::io_context>((int)worker_threads_count);
    auto work = std::make_shared<boost::asio::io_context::work>(*io_context);
    
    boost::thread_group worker_threads;
    for (size_t i = 0; i < worker_threads_count; ++i)
    {
        worker_threads.create_thread(boost::bind(&discnet::app::work_handler, thread_names[i], io_context));
    }

    auto adapter_manager = std::make_shared<discnet::adapter_manager>(std::move(std::make_unique<discnet::windows_adapter_fetcher>()));
    auto route_manager = std::make_shared<discnet::route_manager>(adapter_manager);
    auto multicast_handler = std::make_shared<discnet::app::multicast_handler>(adapter_manager, configuration.value(), io_context);
    auto discovery_handler = std::make_shared<discnet::app::discovery_message_handler>(multicast_handler, route_manager, adapter_manager);
    auto transmission_handler = std::make_shared<discnet::app::transmission_handler>(route_manager, multicast_handler, adapter_manager, configuration.value());

    while (true)
    {
        std::chrono::time_point current_time = discnet::time_point_t::clock::now();
        adapter_manager->update();
        multicast_handler->update();
        route_manager->update(current_time);
        transmission_handler->update(current_time);

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    return EXIT_SUCCESS;
}