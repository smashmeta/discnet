/*
 *
 */

#include <iostream>
#include <map>
#include <ranges>
#include <boost/core/ignore_unused.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/dll.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <fmt/format.h>
#include <discnet/discnet.hpp>
#include <whatlog/logger.hpp>
#include <discnet/node.hpp>
#include <discnet/network/buffer.hpp>
#include <discnet/network/messages/data_message.hpp>
#include <discnet/network/messages/discovery_message.hpp>
#include <discnet/network/messages/packet.hpp>
#include <discnet/network/multicast_client.hpp>
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

    class multicast_handler
    {
        typedef discnet::network::messages::discovery_message_t discovery_message_t;
        typedef discnet::network::messages::data_message_t data_message_t;
        typedef discnet::network::network_info_t network_info_t;
        typedef std::shared_ptr<discnet::network::multicast_client> shared_multicast_client_t;
        typedef std::map<boost::uuids::uuid, shared_multicast_client_t> multicast_client_map_t;
    public:
        multicast_handler(shared_adapter_manager_t adapter_manager, const discnet::app::configuration_t& configuration, discnet::shared_io_context io_context)
            : m_adapter_manager(adapter_manager), m_configuration(configuration), m_io_context(io_context)
        {
            m_adapter_manager->e_new.connect(std::bind(&multicast_handler::adapter_added, this, std::placeholders::_1));
            m_adapter_manager->e_changed.connect(std::bind(&multicast_handler::adapter_changed, this, std::placeholders::_1, std::placeholders::_2));
            m_adapter_manager->e_removed.connect(std::bind(&multicast_handler::adapter_removed, this, std::placeholders::_1));
        }

        void adapter_added(const adapter_t& adapter)
        {
            whatlog::logger log("multicast_handler::adapter_added");
            
            std::string adapter_guid_str = boost::lexical_cast<std::string>(adapter.m_guid);
            log.info("new adapter detected. Name: {}, guid: {}, mac: {}.", adapter.m_name, adapter_guid_str, adapter.m_mac_address);
            add_multicast_client(adapter);
        }

        void adapter_changed(const adapter_t& prev_adapter, const adapter_t& curr_adapter)
        {
            whatlog::logger log("multicast_handler::adapter_changed");

            bool exist_multicast_client = m_clients.find(curr_adapter.m_guid) != m_clients.end();
            bool enabled_changed = prev_adapter.m_enabled != curr_adapter.m_enabled;
            bool ipv4_changed = (prev_adapter.m_address_list.size() != curr_adapter.m_address_list.size()) || 
                !std::equal(prev_adapter.m_address_list.begin(), prev_adapter.m_address_list.end(), curr_adapter.m_address_list.begin());

            // re-create multicast_client if adapter has changed in a significant way
            if (!exist_multicast_client)
            {
                log.info("unknown adapter ({}) appeared. adding multicast client.", curr_adapter.m_name);
                add_multicast_client(curr_adapter);
            }
            else if (enabled_changed || ipv4_changed)
            {
                log.info("adapter ({}) changed. re-creating multicast client.", curr_adapter.m_name);
                remove_multicast_client(curr_adapter);
                add_multicast_client(curr_adapter);
            }
        }

        void adapter_removed(const adapter_t& adapter)
        {
            whatlog::logger log("multicast_handler::adapter_removed");
            remove_multicast_client(adapter);
        }

        void update()
        {
            whatlog::logger log("multicast_handler::update");
            for (auto& client : m_clients | std::views::values)
            {
                client->process();
            }
        }

    private:
        void remove_multicast_client(const discnet::adapter_t& adapter)
        {
            whatlog::logger log("multicast_handler::remove_multicast_client");

            auto itr_client = m_clients.find(adapter.m_guid);
            if (itr_client != m_clients.end())
            {
                std::string adapter_guid_str = boost::lexical_cast<std::string>(adapter.m_guid);
                log.info("removing adapter (name: {}, guid: {}).", adapter.m_name, adapter_guid_str);
                itr_client->second->close();
                m_clients.erase(itr_client);
            }
        }
        void add_multicast_client(const discnet::adapter_t& adapter)
        {
            whatlog::logger log("multicast_handler::add_multicast_client");

            if (!adapter.m_enabled)
            {
                log.info("skipping adapter ({}) because it is disabled.", adapter.m_name);
                return;
            }

            if (adapter.m_address_list.empty())
            {
                log.info("skipping adapter ({}) because it is missing ipv4 address.", adapter.m_name);
                return;
            }

            discnet::network::multicast_info info;
            info.m_adapter_address = adapter.m_address_list.front().first;
            info.m_multicast_address = m_configuration.m_multicast_address;
            info.m_multicast_port = m_configuration.m_multicast_port;
            
            auto shared_client = discnet::network::multicast_client::create(m_io_context, info, 12560);
            bool client_connected = shared_client->open();
            size_t index = 1;
            discnet::time_point_t start_time = std::chrono::system_clock::now();
            discnet::time_point_t current_time = start_time;
            discnet::time_point_t timeout = start_time + std::chrono::seconds(15); 
            while (!client_connected && current_time < timeout)
            {
                log.info("retry connect #{}.", index);
                // give the OS some time to initialize the adapter before we start listening
                std::this_thread::sleep_for(std::chrono::seconds(1));
                client_connected = shared_client->open();
                current_time = std::chrono::system_clock::now();
                ++index;
            }

            if (!client_connected)
            {
                log.error("failed to start multicast client. dropping multicast client on adapter.");
                return;
            }

            log.info("Added adapater to our client map.", info.m_adapter_address.to_string());
            auto [itr_client, inserted] = m_clients.insert(std::pair{adapter.m_guid, shared_client});
            auto [uuid, client] = *itr_client;
        }

        discnet::network::messages::message_list_t get_messages_for_adapter(const discnet::adapter_t& adapter)
        {
            using ipv4 = boost::asio::ip::address_v4;
            using node_t = discnet::network::messages::node_t;
            using discovery_message_t = discnet::network::messages::discovery_message_t;
            using jumps_t = discnet::network::messages::jumps_t;
            using data_message_t = discnet::network::messages::data_message_t;
            using message_list_t = discnet::network::messages::message_list_t;

            boost::ignore_unused(adapter);
            
            discovery_message_t discovery_message{ .m_identifier = m_configuration.m_node_id };
            discovery_message.m_nodes = { node_t{ 1025, ipv4::from_string("192.200.1.1"), jumps_t{512, 256} } };
            data_message_t data_message{ .m_identifier = 1 };
            data_message.m_buffer = { 1, 2, 3, 4, 5 };
            message_list_t messages = { discovery_message, data_message };

            return messages;
        }

        discnet::shared_adapter_manager_t m_adapter_manager;
        discnet::app::configuration_t m_configuration;
        discnet::shared_io_context m_io_context;
        multicast_client_map_t m_clients;
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

    auto fetcher = std::make_unique<discnet::windows_adapter_fetcher>();
    auto adapter_manager = discnet::shared_adapter_manager_t(new discnet::adapter_manager_t(std::move(fetcher)));
    auto multicast_handler = std::make_shared<discnet::app::multicast_handler>(adapter_manager, configuration.value(), io_context);
    while (true)
    {
        adapter_manager->update();
        multicast_handler->update();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    return EXIT_SUCCESS;
}