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
#include <boost/endian.hpp>
#include <discnet/discnet.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
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
#include "asio_context.hpp"
#include "application.hpp"


/*
 *  - persistent node(s)
 *  - transmission_handler: queue
 *  - data_message_handler: message processing
 *  - data_message_handler: message routing
 *  - data_message_t: destination(s)
 *  - data_message_t: replace
 */

void program_yeild(std::shared_ptr<spdlog::logger>& logger, const discnet::time_point_t& start_time)
{
    // sampling program duration
    auto current_time = discnet::time_point_t::clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);

    static int frames_per_seconds = 10;
    static std::chrono::milliseconds milliseconds_per_frame = std::chrono::milliseconds(1000) / frames_per_seconds;
    if (duration < milliseconds_per_frame)
    {
        std::chrono::milliseconds sleep_duration = milliseconds_per_frame - duration;
        std::this_thread::sleep_for(sleep_duration);
    }
    else
    {
        // yield process to not eat too much system time
        logger->info("program execution time {} exceedes maximum frame time {}.", duration.count(), milliseconds_per_frame.count());
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

int main(int arguments_count, const char** arguments_vector)
{
    boost::filesystem::path executable_directory = boost::dll::program_location().parent_path();
    std::cout << "current path is set to " << executable_directory << std::endl;
    
    auto console_logger = spdlog::stdout_color_mt("console_logger");
    discnet::application::expected_configuration_t configuration = discnet::application::get_configuration(arguments_count, arguments_vector);
    if (!configuration)
    {
        console_logger->error("failed to load configuration. terminating application. error: {}.", configuration.error());
        return EXIT_FAILURE;
    }

    auto logger = spdlog::stdout_color_mt(configuration->m_log_instance_id);

    logger->info("configuration loaded. node_id: {}, mc-address: {}, mc-port: {}.", 
        configuration->m_node_id, configuration->m_multicast_address.to_string(), configuration->m_multicast_port);

    discnet::main::application application(configuration.value());
    if (!application.initialize())
    {
        logger->error("failed to initialize application.");
        return EXIT_FAILURE;
    }
    
    logger->info("discnet initialized and running.");
    while (true)
    {
        auto current_time = discnet::time_point_t::clock::now();
        application.update(current_time);   
        program_yeild(logger, current_time);
    }

    spdlog::shutdown();

    return EXIT_SUCCESS;
}
