/*
 *
 */

#include <iostream>
#include <map>
#include <boost/core/ignore_unused.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/dll.hpp>
#include <boost/filesystem.hpp>
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
        log.info(fmt::format("starting thread {}.", thread_name));

        for (;;)
        {
            try
            {
                boost::system::error_code error_code;
                io_context->run(error_code);
                if (error_code)
                {
                    log.error(fmt::format("worker thread encountered an error. message: ", error_code.message()));
                }

                break;
            }
            catch (std::exception& ex)
            {
                log.warning(fmt::format("worker thread encountered an error. exception: {}.", std::string(ex.what())));
            }
            catch (...)
            {
                log.warning("worker thread encountered an unknown exception.");
            }
        }
    }

    class multicast_handler
    {
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
            whatlog::logger log("adapter_added");
            discnet::network::multicast_info info;

            if (!adapter.m_enabled)
            {
                log.warning("adapter disabled.");
                return;
            }

            if (adapter.m_address_list.empty())
            {
                log.warning("added adapter missing valid ip4 address");
                return;
            }

            info.m_adapter_address = adapter.m_address_list.front().first;
            info.m_multicast_address = m_configuration.m_multicast_address;
            info.m_multicast_port = m_configuration.m_multicast_port;
            
            log.info(fmt::format("new adapter detected. IP: {}. Adding adapater to our client map.", info.m_adapter_address.to_string()));
            auto client = std::make_shared<discnet::network::multicast_client>(m_io_context, info, 12560);
            auto [itr_client, inserted] = m_clients.insert(std::pair{adapter.m_guid, client});
            auto [_uuid, _client] = *itr_client;

            if (!inserted || !_client->open())
            {
                log.error("failed to start multicast client");
                return;
            }
        }

        void adapter_changed(const adapter_t&, const adapter_t&)
        {

        }

        void adapter_removed(const adapter_t&)
        {

        }

        void update()
        {
            using ipv4 = boost::asio::ip::address_v4;
            using discnet::node_identifier_t;
            using discnet::network::buffer_t;
            using namespace discnet::network::messages;

            discovery_message_t discovery_message{ .m_identifier = m_configuration.m_node_id };
            discovery_message.m_nodes = {
                node_t{ 1025, ipv4::from_string("192.200.1.1"), jumps_t{512, 256} }
            };
            data_message_t data_message{ .m_identifier = 1 };
            data_message.m_buffer = { 1, 2, 3, 4, 5 };
            buffer_t buffer(1024);
            message_list_t messages = { discovery_message, data_message };
            if (!packet_codec_t::encode(buffer, messages))
            {
                return;
            }

            m_adapter_manager->update();
            for (auto& [identifier, client] : m_clients)
            {
                client->process();
                client->write(buffer);
            }
        }

    private:
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

    discnet::app::multicast_handler multicast_handler(adapter_manager, configuration.value(), io_context);
    while (true)
    {
        multicast_handler.update();
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    return EXIT_SUCCESS;
}