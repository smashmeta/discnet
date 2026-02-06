/*
 *
 */

#include <spdlog/spdlog.h>
#include <discnet/application/configuration.hpp>
#include <discnet/adapter_manager.hpp>
#include <discnet/route_manager.hpp>
#include <discnet/network/network_handler.hpp>
#include <discnet/transmission_handler.hpp>
#include <discnet/discovery_message_handler.hpp>
#include "asio_context.hpp"
#include "application.hpp"
#ifdef _WIN32
#include <discnet/windows/adapter_fetcher.hpp>
#elif defined(__GNUC__) && !defined(__clang__)
#include <discnet/linux/adapter_fetcher.hpp>
#endif


namespace discnet::main
{
    application::application(discnet::application::configuration_t configuration)
        : m_configuration(configuration), m_logger(spdlog::get(configuration.m_log_instance_id))
    {
        // nothing for now
    }

    bool application::initialize()
    {
        m_logger->info("setting up asio network context...");
        m_asio_context = std::make_shared<discnet::main::asio_context_t>(m_configuration);
        m_logger->info("setting up adapter_manager...");

#ifdef _WIN32
        auto adapter_fetcher = std::make_shared<discnet::windows_adapter_fetcher>();
        m_adapter_manager = std::make_shared<discnet::adapter_manager>(m_configuration, adapter_fetcher);
#elif defined(__GNUC__) && !defined(__clang__)
        auto adapter_fetcher = std::make_shared<discnet::linux_adapter_fetcher>();
        m_adapter_manager = std::make_shared<discnet::adapter_manager>(m_configuration, m_loggers, adapter_fetcher);
#endif

        m_logger->info("setting up network_handler...");
        m_network_handler = std::make_shared<discnet::network::network_handler>(m_configuration, m_adapter_manager, std::make_shared<network::client_creator>(m_configuration, m_asio_context->m_io_context));
        m_logger->info("setting up route_manager...");
        m_route_manager = std::make_shared<discnet::route_manager>(m_configuration, m_adapter_manager, m_network_handler);
        m_logger->info("setting up transmission_handler...");
        m_transmission_handler = std::make_shared<discnet::transmission_handler>(m_configuration, m_route_manager, m_network_handler, m_adapter_manager);

        return true;
    }

    void application::update(discnet::time_point_t current_time)
    {
        m_adapter_manager->update();
        m_network_handler->update(current_time);
        m_route_manager->update(current_time);
        m_transmission_handler->update(current_time);
    }
} // !namespace discnet::main